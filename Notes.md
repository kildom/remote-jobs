


# Sample scripts

```javascript


// QuickJS main code draft

import * as os from 'os';
import * as std from 'std';
import * as controller from 'controller'; // native C module that connects to controller
import * as tool from 'tool'; // module loaded before this module that contains tool description generated based on registring data

function main() {
	let r = tool.filter(scriptArgs, tool.filterOptions);
	if (!r) {
  	r = os.exec(...);
    std.exit(r);
  }
  let job = controller.connect(std.getenv('REMOTE_JOBS_PORT') || tool.defaultPort || 3847);
  r = job.send({cmd:'job', args:scriptArgs, scriptHash: tool.scriptHash});
  if (r.cmd === 'getScript') {
  	r = job.send({cmd:'script', text: std.loadFile(tool.scriptFile)});
  }
  while (r.cmd === 'exec') {
    // Format of below list: command, parameters -> task to do -> response, results
    // exec -> execute process, agrs, cwd?, env? -> execResult, exitCode
    // getScript -> read script file -> script, text
    // getEnv, varsNames? -> read selected or all environment variables -> env, vars
    // print, stdout?, stderr? -> print output -> printed
    // exit, exitCode -> exit process -> [no response]
  	r = os.exec(...);
    r = job.send({cmd:'execResult', exitCode: r});
  }
  job.disconnect();
  std.exit(r.exitCode);
}

try {
	main();
} catch (ex) {
  rj.disconnect();
	console.log(ex);
	std.exit(1);
}


////////////////// clang.js

function clangFilter(args) {
  return args.indexOf('-c') >= 0;
}

function clang() {
  if (job.args.find('-c') < 0) return job.execLocal();
  if (job.getPrefferLocal()) return job.execLocal();
  let lastArg = null;
  let output = null;
  for (let arg of job.args) {
    if (lastArg === '-o') {
      output = arg;
    }
    lastArg = arg;
  }
  if (output === null) return job.execLocal();
  let ppArgs = job.args.map(arg => arg === '-c' ? '-E' : arg);
  ppArgs[0] = job.command;
  job.execLocal(ppArgs);
  let res = job.execOnRemote({
  	call: 'clang-remote.js/clangRemote',
    args: {
      input: new job.File(output, true, false),
      output: new job.File(output, false, true),
    },
  });
}

job.register({
	filterFunction: clangFilter,
  execFunction: clang,
  name: 'clang',
  before: 'gcc',
  commands: {
  	'clang': 'clang',
  	'clang++': 'clang++'
  }
});


////////////////// clang-remote.js

function clangRemote(args) {

}


```
