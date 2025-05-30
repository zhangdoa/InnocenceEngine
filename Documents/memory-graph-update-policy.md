# Memory Graph Update Policy

## Purpose
Establish consistent standards for maintaining the project memory graph to ensure accurate project history, lessons learned, and team coordination.

## Core Principles

### 1. Achievement Documentation
- **Work Journals**: Document all completed tasks as memory graph observations
- **Changelist Results**: Record specific commits, files changed, and functionality delivered
- **Milestone Tracking**: Note major architectural improvements and feature completions
- **Format**: Achievement observations should include commit hashes, file counts, and measurable outcomes

### 2. Failure and Mistake Documentation  
- **Learning Records**: Document all failures, mistakes, and incorrect approaches
- **Root Cause Analysis**: Include why the failure occurred and what was learned
- **Prevention Strategies**: Note how to avoid similar issues in the future
- **Format**: Failure observations should include the mistake, impact, and lesson learned

### 3. Memory Graph Maintenance
- **Regular Review**: Periodically review existing observations for compliance with this policy
- **Observation Updates**: Revise existing observations that don't meet current standards
- **Relation Cleanup**: Ensure relations accurately reflect current project state
- **Consistency**: Maintain consistent terminology and formatting across all observations

## Observation Standards

### Achievement Observations Format
```
TASK COMPLETED: [Task Name] - [Commit Hash/Result]
DELIVERABLES: [Specific files/features/functionality delivered]
IMPACT: [How this advances the project goals]
METRICS: [Lines of code, files changed, tests passing, etc.]
```

### Failure/Mistake Observations Format
```
MISTAKE: [Brief description of what went wrong]
CAUSE: [Root cause analysis]
IMPACT: [Consequences of the mistake]
LESSON LEARNED: [How to prevent this in the future]
CORRECTIVE ACTION: [What was done to fix it]
```

### Memory Graph Structure
1. **Entity Hierarchy**: Use nested entity types under main project (Project Milestones, Technical Decisions, Failure Events, Process Improvements)
2. **Cross-Category Relations**: Connect entities across categories to show cause-and-effect and dependencies
3. **Clear Ownership**: All sub-entities clearly belong to the main project through entity types and relations
4. **Scalable Organization**: Structure supports project growth without losing clarity

## Implementation
- All AI Agent Team members must follow this policy when updating memory graph
- User reviews and approvals should also be documented as process improvements
- Policy violations should be corrected immediately when identified
- This document serves as the authoritative source for memory graph standards
- One-time restructure completed 2025-05-30, future updates follow established patterns

## Version History
- v1.0 (2025-05-30): Initial policy establishment

AI-Generated-By: Claude (Anthropic AI Assistant)