/* Honeywell Series 16 emulator
 * Copyright (C) 1997, 1998, 2005, 2026  Adrian Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307 USA
 *
 */
#include "config.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <cinttypes>
#include <format>
#include <cassert>

#include "stdtty.hpp"
#include "proc.hpp"
#include "instr.hpp"
#include "monitor.hpp"

#ifdef ENABLE_GUI
#include "gtk/fp.h"
#endif

/* FP_UPDATE sets how many machine instructions to execute
 * before refreshing the GUI. It is basically a compromise
 * between precise updating of the front-panel display and
 * performance. It is chosen to be not a power of 2 or 10
 * in order to avoid keeping a static display when a program
 * is in a tight loop. */
#define FP_UPDATE 9753

using namespace h16;

#ifdef ENABLE_GUI

static bool gui_get_filename(const std::string &device_name,
                             const std::string &extension,
                             const std::string &description,
                             bool booting,
                             std::string &filename) {
  std::cout << "Hello" << std::endl;
  return "todo";
}


/* fp_run() is the routine that is called by the GUI
 * front-panel to have the machine 'run'. It is installed
 * either as an idle routine, when the machine is
 * continuously running, or called once for a single-step
 * of FP memory access */

static void fp_run(struct FP_INTF *intf) {
  Proc *p = (Proc *) intf->data;
  int count = FP_UPDATE;

  /*
   * If a start button interrupt has been
   * caused then go tell the simulation kernel
   * that this occured.
   */
  if (intf->start_button_interrupt_pending) {
    intf->start_button_interrupt_pending = 0;
    p->start_button();
  }
  
  if (intf->mode == FPM_MA) {
    p->mem_access(intf->p_not_pp1, intf->store);
  } else {
    if (intf->mode == FPM_SI) {
      count = 1;
    }
    
    /*
     * Do machine instructions until the count expires,
     * or a HLT instruction, or the monitor is called for.
     */
    assert(count > 0);
    Proc::DI di;
    do {
      do {
        di = p->do_instr();
        if (di == Proc::DI::FNAM) {
          std::string fn;
          gui_get_filename(p->get_device_name(),
                           p->get_extension(),
                           p->get_description(),
                           p->is_booting(),
                           fn);
            
          di = Proc::DI::HALT; // For now - to avoid infinite loop
        }
      } while (di == Proc::DI::FNAM);
    } while ((di == Proc::DI::OK) && (--count > 0));

    intf->exit_called = (di == Proc::DI::EXIT);

    if (di == Proc::DI::HALT) {
      intf->running = false;
    }
  }
}

/*
 * This is the called when the front-panel master clear
 * button is pressed
 */
static void fp_master_clear(struct FP_INTF *intf) {
  Proc *p = (Proc *) intf->data;
  p->master_clear();
}

#endif

/*
 * Do whatever has to happen in the case that
 * an Alt-key is pressed
 */
static bool special_chars(void *callback_arg, int k) {
  Proc *p = static_cast<Proc *>(callback_arg);
  bool r = false;

  if ( k == ('s' | 0x80)) {
    p->start_button();
    r = 1;
  } else if ( k == ('m' | 0x80)) {
    p->goto_monitor();
    r = 1;
  }
  
  return r;
}

static bool get_filename(const std::string &device_name,
                         const std::string &extension,
                         const std::string &description,
                         bool booting,
                         std::string &filename) {

  StdTty &stdtty {StdTty::getInstance()};
  
  std::string str = std::format("{}: {}> ",
                                device_name,
                                (description.size() == 0) ? "Filename" : description);
  
  stdtty.get_input(str.c_str(), filename, false);

  return (filename.size() > 0);
}

/*
 * Where it all starts...
 */
int main(int argc, char **argv) {
  int exit_code = 0;
#ifdef ENABLE_GUI
  bool front_panel = 1;
#endif
  int arg = 1;
  
  if ((argc>arg) &&
      ((strncmp(argv[arg], "-h", 2)==0) ||
       (strncmp(argv[arg], "--h", 3)==0))) {
    printf("Usage: %s [-h|--h] [-t [<script-file]]\n", argv[0]);
    printf("     : [-h|--h] Prints this help\n");
    printf("     : -t Selects text-only mode. %s\n",
#ifdef ENABLE_GUI
           "(Disables the GUI)"
#else
           "(Assumed, because compiled without GUI support)"
#endif
           );
    printf("     : type \"help\" at \"MON>\" prompt in text-only mode for help on script file commands\n");
    
    exit(0);
  }
  
  /*
   * If the very first option is "-t" then operate
   * in non-GUI mode (text mode) even if compiled with
   * GUI support
   */
  
  if ((argc>arg) && (strcmp(argv[arg], "-t")==0)) {
#ifdef ENABLE_GUI
    front_panel = 0;
#endif
    arg++;
  }
  
#ifdef ENABLE_GUI
  /* Let GTK look at the options */
  if (front_panel) {
    process_args(&argc, &argv);
  }
#endif
  
  /*
   * Handling of the TTY is kind of complicated because
   * it does multiple things (it models the ASR connected to
   * the CPU and it is used to control the simulation and
   * provide the text-based monitor)
   *
   * Initialize the STDTTY and pass it to the newly created
   * Proc (processor), but then go back and tell the STDTTY
   * which Proc it is connected to; they each need to know
   * how to contact the other!
   */
  StdTty &stdtty {StdTty::getInstance()};
  Proc *p = new Proc(true);
  stdtty.register_callback(static_cast<void *>(p), special_chars);

#ifdef ENABLE_GUI
  if (front_panel) {
    /*
     * Build an FP_INTF structure that interfaces
     * the GUI front-panel into the processor.
     * And then setup the front-panel and start
     * simulating...
     */
    struct FP_INTF *intf = p->fp_intf();
    intf->exit_called = false;
    intf->run = fp_run;
    intf->master_clear = fp_master_clear;
    
    setup_fp(intf);
    
    if (intf->exit_called) {
      exit_code = p->get_exit_code();
    }
  } else {
#endif
    /*
     * This is the text-based version
     */
    bool run = 0;
    Monitor *m;
    std::ifstream is;
    
    /*
     * Initialize the monitor
     */
    m = new Monitor(*p, argc, argv);
    
    /*
     * Open a script file (if any)
     * And let the monitor process it.
     */
    if (argc > arg) {
      is.open(argv[arg]);
      if (!is) {
        std::cerr << "Could not open <" << argv[arg] << "> for reading"
                  << std::endl;
        exit(1);
      }
    }
    m->do_commands(run, is);
    
    /*
     * The monitor will return when the processor is to
     * 'run', or to quit the program. While the processor
     * is running simulate instructions, and when the
     * processor stops (or ALT-M is pressed, setting
     * monitor_flag) call the monitor again
     */
    
    while (run) {
      Proc::DI di;
      do {
        di = p->do_instr();
        if (di == Proc::DI::FNAM) {
          std::string fn;
          if (get_filename(p->get_device_name(),
                           p->get_extension(),
                           p->get_description(),
                           p->is_booting(),
                           fn)) {
            
            p->put_filename(fn);
          } else {
            di = Proc::DI::MON;
          }
        }
      } while ((di == Proc::DI::OK) || (di == Proc::DI::FNAM));

      if (di == Proc::DI::HALT) {
        run = false;
      }

      if (di == Proc::DI::EXIT) {
        exit_code = p->get_exit_code();
        // Only print this out in text mode (not GUI)...
        std::ostream &os((exit_code == 0)? std::cout : std::cerr);
        os << std::format("{:0>10d}: exit code = {:d}",
                          p->get_half_cycles(), exit_code) << std::endl;
        run = false;
      } else if ((!run) || (di == Proc::DI::MON)) {
        m->do_commands(run, is);
      }
    }
#ifdef ENABLE_GUI
  }
#endif
  exit(exit_code);
}
