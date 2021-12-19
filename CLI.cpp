/*
   CommandInterface CLI.cpp - base class for Command Line Interfaces
   Copyright (c) 2021 Eric Brundick.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <inttypes.h>
#include <string.h>
#include "CLI.h"


CommandInterface::CommandInterface(const CLICommand *manif) {
  _inputBuffer = NULL;
  _inputBufferLength = 0;
  _manifest = (CLICommand *)manif;
  _cmd_ready = false;
  _stream = NULL;
  int i;
  for (i=0; i < CLI_MAX_ARGUMENTS; i++) {
    _arguments[i] = NULL;
  }
  _argc = 0;
}

void CommandInterface::begin(Stream * s, char * inbuf, unsigned int inbufLength) {
  if (_manifest == NULL || s == NULL || inbuf == NULL || inbufLength < 2) {
    return;
  }
  _stream = s;
  _inputBuffer = inbuf;
  _inputBufferLength = inbufLength;
  _cmd_ready = false;
  _argc = 0;
  _head = _inputBuffer;
  *_head = '\0';
  _arguments[0] = _head;
  _ignore_spaces = false;
}

bool CommandInterface::_head_record(const char c) {
  if ( (_head - _inputBuffer) < (int)_inputBufferLength ) {
    *_head++ = c;
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
    _stream->print("recorded: "); _stream->println(c);
#endif
    return true;
  } else {
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
    _stream->print("unable to record due to buffer full: "); _stream->println(c);
#endif
    return false;
  }
}

bool CommandInterface::processInput(void) {
  if (_cmd_ready) {
    return true;  // This function always returns true until executeInput has been run after a successful line parse.
  }
  // No pending command ready to execute; proceed to process stream input & parse as necessary
  while (_stream->available() > 0) {
    char c = (char)_stream->read();

    // If we've maxed out the arg count, spaces & double-quotes get treated as ordinary data down below.
    if (_argc < CLI_MAX_ARGUMENTS) {
      if (c == '"') {
        _ignore_spaces ^= true;  // Double-quotes let you specify arguments with spaces inside
        if (!_ignore_spaces) {  // Closing a double-quote section
          // Record a '\0' and advance _arguments, _argc
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
          _stream->print("argc at closing double quote: "); _stream->println(_argc);
#endif
          _argc++;
          if (_argc < CLI_MAX_ARGUMENTS) {
            _head_record('\0');
            _arguments[_argc] = _head;
          }
          continue;
        } else {
          // Do not record the " in the buffer, keep the argv pointing where it is
          continue;
        }
      }
      if (!_ignore_spaces && (c == ' ' || c == '\t') ) {
          // Process this in _arguments & _argc
          if (_arguments[_argc] == _head) {  // Ignore multiple consecutive spaces
            continue;
          }
          _argc++;
          if (_argc < CLI_MAX_ARGUMENTS) {
            _head_record('\0');
            _arguments[_argc] = _head;
          } else { // If _argc is maxed out, all text should be recorded in the final argument
            _head_record(c);
          }
          continue;
      }
    }
    if (c == '\r') {
      // Set _cmd_ready to true, we are done.  Zero-out the head.  Return true
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
      _stream->println("detected carriage return- finishing cmd");
#endif
      if ( (_head - _inputBuffer) == (int)_inputBufferLength ) {
        // We have a problem!  Can't record the '\0' without overrunning the buffer.  Zero out the previous byte instead.
        *(_head - 1) = '\0';
      } else {
        *_head = '\0';
      }
      if (_argc < CLI_MAX_ARGUMENTS && _arguments[_argc] < _head) {
        _argc++;
      }
      _head = _inputBuffer;
      _ignore_spaces = false;
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
      unsigned int i;
      for (i=0; i < _argc; i++) {
        _stream->print("arg #"); _stream->print(i); _stream->print(": "); _stream->println(_arguments[i]);
      }
#endif
      _cmd_ready = true;
      return true;
    }
    if (c == '\n') {  // Ignored characters
      continue;
    }
    // Ordinary data - keep stuffing it into the input buffer and advance _head
    _head_record(c);
  }
  return false;  // Data may or may not have been parsed but a full command line is not ready yet
}

void CommandInterface::_input_reset() {
  _argc = 0;
  _head = _inputBuffer;
  _arguments[_argc] = _head;
  _ignore_spaces = false;
  _cmd_ready = false;
}

bool CommandInterface::executeInput() {
  if (!_cmd_ready) {
    return false;
  }
  if (_argc > 0) {
    const char * cmd = _arguments[0];
    CLICommand *cmdhead = _manifest;
    while (cmdhead->cmd != NULL) {
      if (strcmp(cmdhead->cmd, cmd) == 0) {
        // Found a valid command!  Execute it.
        cmdhead->handler_func((int)_argc - 1, (const char **)(_arguments+1), _stream, this, cmdhead->handler_data);
        _input_reset();
        return true;
      }
      cmdhead++;
    }
  }
  _input_reset();
  return false; // Command not found
}

bool CommandInterface::processSubCommand(const CLICommand *subcmds, const int argc, const char **argv) {
  if (argc >= 0) {
    const char *cmd = argv[0];
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
    _stream->print("Processing subcommand: "); _stream->println(cmd);
#endif
    CLICommand *cmdhead = (CLICommand *)subcmds;
    while (cmdhead->cmd != NULL) {
      if (strcmp(cmdhead->cmd, cmd) == 0) {
        // Found a valid command!  Execute it.
#if defined(COMMANDINTERFACE_DEEP_DEBUG) && COMMANDINTERFACE_DEEP_DEBUG > 0
        _stream->print("Processing with argc = "); _stream->print(argc-1); _stream->print(", argv[0] = ");
        _stream->println(argv[1]);
#endif
        cmdhead->handler_func(argc - 1, (const char **)(argv+1), _stream, this, cmdhead->handler_data);
        return true;
      }
      cmdhead++;
    }
  }
  return false; // Command not found
}