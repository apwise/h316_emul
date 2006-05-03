m4_define(`M4H_DEFAULT_HEADING', `DAP-16 Code') m4_dnl
m4_define(`M4H_DEFAULT_TOC_TITLE', `Table of Contents') m4_dnl
m4_define(`M4H_HEAD_TAG', `h1') m4_dnl
m4_define(`M4H_HEAD_TAG_END', `/h1') m4_dnl
m4_define(`M4H_TABLE_START',`<TABLE WIDTH="100%" class="simple"> <col WIDTH="20%"><col WIDTH="20%"><col WIDTH="20%"><col WIDTH="20%"><col WIDTH="20%"> <tbody> <TR>') m4_dnl
m4_define(`M4H_TABLE_STOP',`</TR> </tbody> </table>') m4_dnl
m4_define(`M4H_BUTTON_START',`<TD class="head">') m4_dnl
m4_define(`M4H_BUTTON_STOP',`</TD>') m4_dnl
m4_define(`M4H_BUTTON_EMPTY',`<TD class="head">&nbsp;</TD>') m4_dnl
m4_define(`M4H_LINK_START',`<LINK ') m4_dnl
m4_define(`M4H_LINK_STOP',`>') m4_dnl
m4_dnl
m4_define(`M4H_NEXT_PAGE',`Next&nbsp;Page') m4_dnl
m4_define(`M4H_PREVIOUS_PAGE',`Previous&nbsp;Page') m4_dnl
m4_define(`M4H_NEXT_FILE',`Next&nbsp;File') m4_dnl
m4_define(`M4H_PREVIOUS_FILE',`Previous&nbsp;File') m4_dnl
m4_define(`M4H_SINGLE_PAGE',`Single&nbsp;Page') m4_dnl
m4_define(`M4H_MULTIPLE_PAGES',`Multiple&nbsp;Pages') m4_dnl
m4_dnl
m4_define(`M4H_HEAD1', `<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
   "http://www.w3.org/TR/html4/strict.dtd">
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=us-ascii">
    <link rel="STYLESHEET" type="text/css" href="main.css" title="Style">
    <link rel="STYLESHEET" type="text/css" href="dap.css" title="Style">') m4_dnl
m4_define(`M4H_HEAD2', `   <title>HTML_HEADING</title>
  </head>
  <body>
    <DIV ID="header">
    <M4H_HEAD_TAG>HTML_HEADING</M4H_HEAD_TAG>
    </DIV>
    <DIV ID="content">') m4_dnl
m4_define(`M4H_HEAD3', `      <div class="code">') m4_dnl
m4_define(`M4H_TAIL1', `  </div>') m4_dnl
m4_define(`M4H_TAIL2', `  </div>
  <div id="footer">
        <P>HTML generated HTML_TIME</P>    
  </div>
  </BODY>
</HTML>') m4_dnl
