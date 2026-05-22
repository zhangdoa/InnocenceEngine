// agent-dispatch: block Agent calls dispatched foreground without [foreground-required] sentinel in prompt.

const FOREGROUND_SENTINEL = '[foreground-required]'

function isAgentDispatch(toolName) {
  return toolName === 'Agent' || toolName === 'Task'
}

function run(input) {
  if (!isAgentDispatch(input.tool_name)) return { ok: true }

  const ti = input.tool_input || {}
  if (ti.run_in_background === true) return { ok: true }

  const prompt = typeof ti.prompt === 'string' ? ti.prompt : ''
  if (prompt.includes(FOREGROUND_SENTINEL)) return { ok: true }

  return { ok: false, block: () => emit(ti) }
}

function emit(ti) {
  const subagent = ti.subagent_type || '<unspecified>'
  const desc = ti.description || '<no description>'
  process.stderr.write([
    '',
    '[session-gate] Agent dispatch blocked — foreground call without justification.',
    '',
    `  subagent_type: ${subagent}`,
    `  description:   ${desc}`,
    `  run_in_background: ${ti.run_in_background === undefined ? '<unset>' : String(ti.run_in_background)}`,
    '',
    'Default is `run_in_background: true`. Foreground requires `[foreground-required]` in the prompt.',
    '',
    'Foreground justification:',
    '  1. Dispatcher\'s immediate next action depends on the sub-agent\'s result, AND',
    '  2. No parallel work the dispatcher could be doing while the sub-agent runs.',
    '',
    'Otherwise retry with `run_in_background: true`.',
    '',
  ].join('\n'))
  process.exit(2)
}

module.exports = { run, FOREGROUND_SENTINEL }
