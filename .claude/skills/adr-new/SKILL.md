---
name: adr-new
description: Create a new Architecture Decision Record (ADR) under docs/adr/. Use when the user says "写一个 ADR / 新增 ADR / 加 ADR / 记录架构决策 / 这个决策需要 ADR". Auto-finds the next free ADR number, generates a populated template (Status / Context / Decision / Consequences / Alternatives Considered) with the title and topic the user gives, sets initial Status to "Proposed", and creates the file. Does NOT modify ROADMAP.md / ARCHITECTURE.md cross-references — those are separate manual steps.
---

# adr-new

## When to invoke

Triggers (any of):

- User says: "写一个 ADR / 加 ADR / 新增 ADR / 记录这个决策 / ADR-XXXX 关于 ..."
- After a major design discussion that locks in a non-trivial decision (technical choice, library, architectural boundary).

Do **not** invoke when:

- The decision is reversible / easily changed (use a comment or commit message).
- The "decision" is actually a TODO / followup → use `docs/inbox.md` instead.
- The user just wants to brainstorm — let them brainstorm; only invoke after they say "let's commit to it".

## Procedure

1. **Find the next free ADR number**:
   ```bash
   ls docs/adr/ADR-*.md | tail -1
   # extracts e.g. "ADR-0009-self-host-roadmap.md" → next is 0010
   ```
   If none exist, start at 0001.

2. **Ask user for** (if not already given):
   - **Title** — short noun phrase, kebab-case in filename (e.g., `drawing-domain-boundary`)
   - **Status** — usually `Proposed` initially. Other options: `Accepted` / `Deprecated` / `Superseded by ADR-XXXX`
   - **Context** — what's the problem / forcing function?
   - **Decision** — what we chose to do
   - **Consequences** — both positive and negative (be honest about trade-offs)
   - **Alternatives** — what we considered + why rejected (this is the most valuable part for future readers)

3. **Generate the file** at `docs/adr/ADR-NNNN-<kebab-title>.md` using the template at [docs/adr/ADR-template.md](../../../docs/adr/ADR-template.md). Structure:
   ```markdown
   # ADR-NNNN: <Title>

   - **Status**: Proposed | Accepted | Deprecated | Superseded by ADR-XXXX
   - **Date**: YYYY-MM-DD
   - **Deciders**: <names / GitHub handles>
   - **Tags**: <comma-separated, e.g. domain, drawing, vendor-selection>

   ## Context

   <What's the situation? What forcing function triggered this decision?>

   ## Decision

   <The chosen option, stated declaratively. Future readers should be able
    to understand WHAT we decided just by reading this section.>

   ## Consequences

   ### Positive
   - ...

   ### Negative
   - ...

   ### Neutral / followups
   - ...

   ## Alternatives Considered

   ### Alt 1: <name>
   <description, why rejected>

   ### Alt 2: <name>
   <description, why rejected>

   ## References

   - Related ADRs: ADR-XXXX
   - Related docs: docs/architecture/...
   - External: <links>
   ```

4. **Cross-reference** (manual, not automated):
   - If this ADR supersedes a prior one, edit the prior ADR's Status to `Superseded by ADR-NNNN`.
   - If this ADR closes an item from `docs/inbox.md`, strikethrough that inbox entry and link to the new ADR.
   - Mention to user: "Should I also update ROADMAP.md / ARCHITECTURE.md to reference this ADR?" — do **not** do it without explicit request.

5. **Tell the user** the file path + suggest next steps (e.g., "commit message template: `docs(adr): record ADR-NNNN <title>`").

## Validation

- ADR number is unique (no collision with existing files)
- Filename kebab-case, no spaces/uppercase
- Status starts at `Proposed` unless user explicitly says otherwise
- All 5 sections present (even if some say `_TBD_`)

## Anti-patterns

- ❌ Don't make decisions for the user. ADRs document what they decided, not what AI thinks is best.
- ❌ Don't skip the "Alternatives Considered" section even if there's only one — an ADR without alternatives looks like it never had real options.
- ❌ Don't auto-commit. Let the user review.

## Related

- Template: [docs/adr/ADR-template.md](../../../docs/adr/ADR-template.md)
- Index: [docs/adr/README.md](../../../docs/adr/README.md)
- Existing ADRs: 0001 (LGPL) / 0002 (domain zero deps) / 0003 (events immutable) / 0004 (OCCT) / 0005 (OpenGL not Vulkan) / 0006 (plugin double ABI) / 0007 (EnTT) / 0008 (VS toolchain) / 0009 (self-host roadmap)
