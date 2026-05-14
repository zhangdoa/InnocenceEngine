// Sub-agent transcript + agent-manifest helpers — used by the
// skill-evidence gate (and the older Bash-only resolver in commit-gate
// via a delegation in lib/common.js). Kept separate from lib/common.js
// so common.js stays under the 300-line file-size gate.
//
// Cross-cutting infrastructure (regexes, basic transcript scanning,
// commit-message parsing) stays in common.js. Per-domain helpers (the
// sidechain-matching topology, agent-manifest parsing) live here.

const fs = require('fs')

// Generalized form of `resolveActiveTranscriptPath` (commit-gate's
// Bash-only resolver): match the in-flight tool call against the last
// assistant tool_use in each sub-agent transcript via a caller-supplied
// predicate. Fail-open: ambiguous or absent match → return parent path.
//
// Used by the skill-evidence gate to locate a sub-agent transcript on
// any tool kind (Write/Edit/NotebookEdit, not just Bash). The matcher
// receives the candidate tool_use block; the gate supplies an input-
// equality check so the matching is as strict as the Bash-command match.
function resolveActiveSubagentTranscript(xpFromHook, matchToolUse) {
  if (!xpFromHook || typeof matchToolUse !== 'function') return xpFromHook
  const path = require('path')
  const base = path.basename(xpFromHook)
  if (!base.endsWith('.jsonl')) return xpFromHook
  const stem = base.slice(0, -'.jsonl'.length)
  const subDir = path.join(path.dirname(xpFromHook), stem, 'subagents')
  let entries
  try { entries = fs.readdirSync(subDir) } catch { return xpFromHook }
  const candidates = entries.filter(f => f.endsWith('.jsonl'))
  const matches = []
  for (const name of candidates) {
    const full = path.join(subDir, name)
    let raw
    try { raw = fs.readFileSync(full, 'utf8') } catch { continue }
    const lines = raw.split('\n').filter(Boolean)
    if (lines.length === 0) continue
    // Tail-first scan for the last assistant tool_use block.
    let matched = false
    for (let i = lines.length - 1; i >= 0 && !matched; i--) {
      let row
      try { row = JSON.parse(lines[i]) } catch { continue }
      const role = row.type || row.role || row.message?.role
      if (role !== 'assistant') continue
      const content = row.message?.content
      if (!Array.isArray(content)) continue
      let sawToolUse = false
      for (const b of content) {
        if (b?.type !== 'tool_use') continue
        sawToolUse = true
        if (matchToolUse(b)) { matched = true; break }
      }
      // The "last assistant tool_use" is the one we compare against.
      // If a tool_use row was found and none matched, stop — older rows
      // are stale relative to the in-flight call.
      if (sawToolUse) break
    }
    if (matched) matches.push(full)
  }
  if (matches.length === 1) return matches[0]
  return xpFromHook
}

// Map a sub-agent's hashed `agentId` (filename stem under
// `subagents/agent-<id>.jsonl`) back to its `subagent_type` by scanning
// the PARENT transcript for the `Agent` tool_use → tool_result pair that
// spawned this sidechain. The tool_result content contains a text block
// of the form `agentId: <hash>`; the matching tool_use carries the
// `subagent_type`. Returns null if unresolved (gate fail-open trigger).
function mapAgentIdToSubagentType(parentXp, agentId) {
  if (!parentXp || !agentId) return null
  let raw
  try { raw = fs.readFileSync(parentXp, 'utf8') } catch { return null }
  let toolUseId = null
  const idMarker = 'agentId: ' + agentId
  const lines = raw.split('\n')
  for (const line of lines) {
    if (!line) continue
    let m
    try { m = JSON.parse(line) } catch { continue }
    const content = m.message?.content
    if (!Array.isArray(content)) continue
    for (const block of content) {
      if (block?.type !== 'tool_result') continue
      const inner = block.content
      if (!Array.isArray(inner)) continue
      for (const t of inner) {
        if (t?.type !== 'text' || typeof t.text !== 'string') continue
        if (t.text.includes(idMarker)) { toolUseId = block.tool_use_id; break }
      }
      if (toolUseId) break
    }
    if (toolUseId) break
  }
  if (!toolUseId) return null
  for (const line of lines) {
    if (!line) continue
    let m
    try { m = JSON.parse(line) } catch { continue }
    const content = m.message?.content
    if (!Array.isArray(content)) continue
    for (const block of content) {
      if (block?.type !== 'tool_use') continue
      if (block.id !== toolUseId) continue
      const t = block.input?.subagent_type
      if (typeof t === 'string' && t.length > 0) return t
    }
  }
  return null
}

// Manifest "Always-apply skills" parser. The three impl-stage manifests
// (code-impl, shader-impl, harness-impl) declare always-apply skills in
// a uniform shape on a body line: `Always-apply skills: \`a\`, \`b\`.
// User-level: \`x\`.` followed optionally by `On commit: …` and other
// phase-scoped clauses on the same line. Both project-level and
// `User-level:` halves count as always-apply; `On commit:` / `On bug:`
// are conditional and must NOT be required pre-Write.
//
// Strategy: capture the substring up to the first `. <CapitalizedPhrase>:`
// transition that is NOT `User-level:`. Pull every backtick-wrapped
// token from that span. Claude Code's Skill tool resolves user-level
// skills by bare name, so the gate treats them identically.
//
// Returns string[] of skill names on success, null on read / parse
// failure (gate fail-open trigger).
// Soft-wrap caveat: if a manifest reflows the always-apply line so it
// continues with `\nUser-level:`, ALWAYS_APPLY_RE's `\n[A-Z]` terminator
// matches before the CONDITIONAL_PHRASE_RE exemption can fire, truncating
// the capture early. Current manifests keep User-level on the same line;
// reflow audit lands in the v2 conditional-skills extension.
const ALWAYS_APPLY_RE = /^Always-apply skills:\s*(.+?)(?:\r?\n\r?\n|\r?\n[A-Z]|$)/ms
const CONDITIONAL_PHRASE_RE = /\.\s+(?!User-level:)[A-Z][a-zA-Z-]*(?:\s+[a-z]+)?:/
function parseAlwaysApplySkills(manifestPath) {
  let body
  try { body = fs.readFileSync(manifestPath, 'utf8') } catch { return null }
  const m = body.match(ALWAYS_APPLY_RE)
  if (!m) return null
  let span = m[1]
  const cut = span.search(CONDITIONAL_PHRASE_RE)
  if (cut >= 0) span = span.slice(0, cut)
  const names = []
  const tokenRe = /`([^`]+)`/g
  let t
  while ((t = tokenRe.exec(span)) !== null) names.push(t[1])
  return names.length === 0 ? null : names
}

// Scan a transcript JSONL for `Skill` tool-use entries. Returns a
// Set<string> of skill names invoked, or null on I/O error.
//
// The Skill tool's `input` is shaped `{ skill: '<name>', args?: '...' }`
// per the harness contract; the body of the SKILL.md enters the model's
// context window when the tool returns, so its presence in the transcript
// is the cheapest proof the agent has the content available.
function scanTranscriptForSkillUses(xpPath) {
  let raw
  try { raw = fs.readFileSync(xpPath, 'utf8') } catch { return null }
  const found = new Set()
  for (const line of raw.split('\n')) {
    if (!line) continue
    let m
    try { m = JSON.parse(line) } catch { continue }
    const content = m.message?.content
    if (!Array.isArray(content)) continue
    for (const block of content) {
      if (block?.type !== 'tool_use') continue
      if (block.name !== 'Skill') continue
      const skill = block.input?.skill
      if (typeof skill === 'string' && skill.length > 0) found.add(skill)
    }
  }
  return found
}

module.exports = {
  resolveActiveSubagentTranscript,
  mapAgentIdToSubagentType,
  parseAlwaysApplySkills,
  scanTranscriptForSkillUses,
  ALWAYS_APPLY_RE,
  CONDITIONAL_PHRASE_RE,
}
