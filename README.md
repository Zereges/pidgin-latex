# pidgin-latex
Reimplementation of popular pidgin plugin used to display LaTeX formulas directly to IM chat.

### Requirements
- Latex (in `PATH`)
- [Dvipng](http://savannah.nongnu.org/projects/dvipng/) (in `PATH`)

On windows `dvipng` is usually part of texlive installation, which is what most windows users use.  
On unix, if you don't have dvipng installed, use your distro's package manager (*aptitude*, *pacman*) to install it.

Both platforms usually add those programs to `PATH` environment variable automaticaly.

### Usage
Plugin searches for text between `$$` and `$$`. When found, this text is substituted for `%TEXT%` in this file

    \documentclass[preview]{standalone}
    \usepackage{amsmath}
    \usepackage{color}
    \usepackage{amsfonts}
    \usepackage{amssymb}
    \definecolor{bgcolor}{RGB}{%SETTINGS_BG%}
    \definecolor{fgcolor}{RGB}{%SETTINGS_FG%}
    \begin{document}
      \setlength{\\fboxsep}{0pt}
      \colorbox{bgcolor}{
        \color{fgcolor}
        $%TEXT%$
      }
    \end{document}

Which is then processed by `latex` to create *dvi* file. Such file is then processed with `dvipng` to create `png` image containing desired output. That image is inserted directly into a displayed message.

Plugin is enabled in IM chats, not in group chats (like IRC).

Plugin is configurable directly from pidgin GUI. It has following options
- background color: In format `R,G,B`. It's substituted for `%SETTINGS_BG%`. Default value is white.
- foreground color: In format `R,G,B`. It's substituted for `%SETTINGS_FG%`. Default value is black.
- font size: Desired font size of formula in the image. Default value is 9px.

### Download
Windows users can download plugin from [this link](https://github.com/Zereges/pidgin-latex/releases/download/v1.2/LaTeX.dll).  
Unix users have to compile it, package is not available.

### Installation
Place library file (`.dll` or `.so`) to `path-to-pidgin/plugins/` (all users) or `.purple/plugins` (current user) and turn the plugin on in pidgin.

### Compilation
In order to compile the plugin, you have to compile pidgin from sources (at least *libpurple*). See [pidgin wiki](https://developer.pidgin.im/wiki#DevelopmentInformation) for instructions.

Once *libpurple* is compiled, place `LaTeX.c` and `LaTeX.h` to `pidgin-2.x.y/libpurple/plugins` and execute

    make LaTeX.so

On windows platform, execute

    make -f Makefile.mingw LaTeX.dll

instead.

### ToDo
- Retrieve bgcolor, fgcolor and font size from gtk settings, pidgin preferences or directly from message, whichever has higher priority.
- Figure out how to resolve quite long (1second) Latex and dvipng calls which causes pidgin to stop responding.

### Bugs
Please report any bugs you found to zereges&lt;at&gt;ekirei.cz or message me directly at github. Attach debug widnow output or screenshots if necessary.

### License
This program is licensed under MIT licence. For full text of the license, see `./LICENSE` file in this repository.
