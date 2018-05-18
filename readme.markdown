# Vimifier
Vimifier translates Vim-style input and emulates Vim behavior using common
keystroke sequences that most Windows edit controls recognize. To the extent
that those applications implement basic text editing features, Vimify can make
the application behave like Vim does.

Vimifier works with applications like Microsoft Word, Google Chrome, Visual
Studio (watch out, VsVim!) and more. For example, this README was initially
composed in `notepad.exe` with Vimify attached.

## Usage
Execute `vimify.exe`, then click inside the window and drag the pointer to the
application you want to Vimify. Alternately, execute `vimify.exe <pid>` to
Vimify an already-running application.

## How It Works
Vimifier installs a low-level keyboard hook to intercept keystrokes sent to the
application you choose. When Vimifier intercepts a keystroke like `j`, it
suppresses the key down event and issues the down arrow (`VK_DOWN`) instead.
For more complex operations, Vimifier issues more keystrokes. For instance, the
`w` command requires the cursor to move to the beginning of the next word. To
do this, Vimifier issues Ctrl+Right. To select text, Vimifier uses the SHIFT
key along with movement keystrokes. To cut, it uses Ctrl+X. For multi-key
commands such as `gg`, Vimifier's command mode implements a state machine that
awaits and responds to the next key.

## Interesting Capabilities
Vimifier can record macros with the `q` command, and when replaying them,
Vimifier injects all the same keystrokes that were sent. Interestingly, if you
enter insert mode such as by issuing the `i` command, you can send most hotkeys
that applications use. This means Vimifier supports macros that are specific to
the application you are using just be replaying your keystrokes. For instance,
in Microsoft Word, the following keystrokes (separated by gratuitous linefeeds
and comments for readability) would change the font of the word following the
cursor to Chiller:

```
	qc             // Record macro to register `c`
	v              // Enter select mode
	e              // Move to end of word
	i              // Enter insert mode (allows normal app shortcuts)
	Ctrl+D         // Open the Font dialog (specific to MS Word)
	Chiller<Enter> // Type the font name and dismiss the font dialog
	Escape         // Return to command mode
	q              // Terminate macro recording
```

After recording this macro, you can invoke it by typing `@c`.

## Idiosynchrasies and Limitations
Vimifier is a bit of a hack. It's limited by the fact that it mostly can't
account for the contents of the edit buffer. For instance, Vimifier emulates
the `a` command (append at end of line) by issuing the right arrow followed by
entering insert mode to conform to Vim's behavior of moving the cursor to the
right of the character where the cursor was when the command was issued.
Emulating this on an empty line will cause the cursor to move to the next line
before entering insert mode. Woops!

Vimifier is also limited by the capabilities of the application. For instance,
whereas the real Vim supports multiple undo, Windows Notepad does not.
Therefore, since Vimifier's implementation of the `u` (undo) command is to
issue the keystroke Ctrl-Z, a Vimified Notepad will merely toggle before and
after the last change made to the buffer.

Vimifier won't work well with `cmd.exe` because, even on Windows 10, the
command interpreter and console host do not support all the standard keystrokes
that most edit controls do.

I wrote Vimifier as I went on vacation, so there are a few other things I just
wasn't determined to support or fix at the time. For instance, Vimifier doesn't
work correctly when you ask it to record and replay a macro that contains an
invocation of another macro. Shrug.

## Goals
Some commands I could implement right now:
* `>>`

One limitation I'd like to rectify with further development is that Vimifier
doesn't currently implement commands like `~` (toggle case) or `f`/`F` (find
forward/backward) because there is no generic equivalent keystroke to do these
in most Windows applications. This could be rectified with better clipboard
support to allow Vimifier to preserve and restore the state of the global
clipboard and then use clipboard operations to sample the contents of the
buffer around the cursor.

Some items that depend on improved clipboard support:
* `~`
* `f`, `F`, `,`, `;`
* `<<` (have to read the whitespace to be able to mess with it)
