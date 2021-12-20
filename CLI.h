/*
   CommandInterface CLI.h - base class for Command Line Interfaces
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

#pragma once

#include <Stream.h>

#define CLI_MAX_ARGUMENTS 32

#define COMMANDINTERFACE_DEEP_DEBUG 0

class CommandInterface;

typedef struct {
  const char * cmd;  // Command string
  // Handler function takes argc, argv, the Stream (for printing), and optional data (void *)
  bool (*handler_func)(int argc, const char ** argv, Stream *, CommandInterface *, void *);
  void *handler_data;  // Arbitrary data to pass to handler_func's 2nd argument
} CLICommand;

class CommandInterface
{
  private:
    char * _inputBuffer;
    unsigned int _inputBufferLength;
    char * _arguments[CLI_MAX_ARGUMENTS];
    unsigned int _argc;
    bool _ignore_spaces;
    CLICommand * _manifest;
    volatile bool _cmd_ready;
    Stream * _stream;
    char *_head;
    void _input_reset(void);  // Utility function to reset _head, _argc et al
    bool _head_record(const char);  // Record a byte to input buffer with length validation
                                    // Returns true if byte could be recorded, false if buffer is full.
  public:
    CommandInterface(const CLICommand *);
    void begin(Stream *, char *, unsigned int sz);  // Initialize the Command Line Interface based around a Stream object
    // ^ begin() requires providing a character buffer of specified size for collecting input.
    bool processInput();  // Run underlying Stream.available() and process bytes, parsing arguments
                          // Returns true if an input string was processed & is ready to execute.  Execution of command
                          // is deferred to executeInput().  This function is callable from Interrupt handlers.
                          // If executed with true return value prior & command still hasn't been executed, it will
                          // return true and exit immediately without draining any Stream input bytes.

    bool executeInput();  // Execute pending input command.  This function is not intended to be called from an Interrupt
                          // handler but is used to followup from a processInput() that was called via Interrupt routine
                          // in a general sleep state. (Usually in this situation, processInput() returning true
                          // results in the processor being awoken from a deeper sleep state.)
                          // Returns true if the command was a valid command, false if command not found.
    bool processSubCommand(const CLICommand *, const int argc, const char **argv);
                          // ^ Callable from inside a command callback function, this can process sub-commands.
                          // The user can define "sub-command" manifests as separate CLICommand arrays passed to this
                          // function's 1st argument.
};