// Sub-agent transcript + agent-manifest helpers. Used by skill-evidence gate and
// commit-gate's sidechain resolver (via lib/common.js).

const fs = require('fs')

// Match the in-flight tool call against the last assistant tool_use in each sub-agent
// transcript. Fail-open: ambiguous or absent match → return parent path.
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
      if (sawToolUse) break
    }
    if (matched) matches.push(full)
  }
  if (matches.length === 1) return matches[0]
  return xpFromHook
}

// Map sub-agent hashed agentId back to subagent_type via parent transcript scan.
// The Agent tool's result content carries `agentId: <hash>`; the matching tool_use carries subagent_type.
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

// Parse manifest's "Always-apply skills:" line. Captures backtick-wrapped names up to the
// first ". <CapitalizedPhrase>:" transition that is NOT "User-level:". On-commit / on-bug
// clauses are conditional, not always-apply, so excluded.
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
