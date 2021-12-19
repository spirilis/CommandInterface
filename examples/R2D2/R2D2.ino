#include <CLI.h>
#include <string.h>

bool run_help(int argc, const char **argv, Stream *out, CommandInterface *cif, void *extra) {
  out->println("Help text:");
  out->println("help  - request a list of commands");
  out->println("All commands:");
  CLICommand *cmd = (CLICommand *)extra;
  while (cmd->cmd != NULL) {
    out->println(cmd->cmd);
    cmd++;
  }
  return true;
}

const CLICommand r2d2_subs[] = {
  {"scream", run_r2d2, (void *)"AIE DEE DOO DOO"},
  {"zap", run_r2d2, (void *)"ZAP!!!"},
  NULL
};
bool run_r2d2(int argc, const char **argv, Stream *out, CommandInterface *cif, void *extra) {
  if (argc > 0 && extra == NULL) {
    cif->processSubCommand(r2d2_subs, argc, argv);
  } else {
    if (extra != NULL) {
      out->println((const char *)extra);
    } else {
      out->println("OH BE QUIET C3PO");
    }
  }
  return true;
}

const CLICommand cmds[] = {
  {"help", run_help, (void *)&cmds},
  {"r2d2", run_r2d2, NULL},
  NULL
};

CommandInterface cli(cmds);
char buf[2048];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  cli.begin(&Serial, buf, 2048);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (cli.processInput()) {
    cli.executeInput();
  }
}
