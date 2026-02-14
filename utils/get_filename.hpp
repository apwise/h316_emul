/* Honeywell Series 16 emulator
 *
 * Copyright (C) 2026  Adrian Wise
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
 */

#ifndef _GET_FILENAME_HPP_
#define _GET_FILENAME_HPP_

#include <string>
#include "get_filename_intf.hpp"

namespace h16 {
  class GetFilename : public GetFilenameIntf {
  public:
    std::string get_file_name(const std::string &device_name,
                              const std::string &extension,
                              const std::string &description);
  };
}
#endif // _GET_FILENAME_HPP_
