# Unified Allocation Engine (Cells + Dining) — Design

Status: DRAFT for review · Date: 2026-09-13 · Scope: DIPI `dh_manageapp`

## 1. Context & motivation

DIPI assigns two kinds of numbered resources to course attendees: **meditation cells**
and (new, not yet shipped) **dining seats**. Today:

- `generate_cell_list()` (inc/zero-day.inc, ~180 lines) tangles config parsing,
  old/new splitting, reserved-exclusion via manual index-walking, and batch/shared-cell
  cycling into one function. It is effectively unverifiable by reading.
- Evidence it is unreliable: **Dhamma Giri** has 81 in-range "reserved" cells assigned
  to *non-fixed* students even though auto-allocation ran (453 batch rows). Cannot be
  cleanly explained as bug vs "reserved list edited after the last allocation".
- Cells have **no group-wise** support. Dining was just built with group-wise on a
  *separate* allocator — two near-identical code paths that will drift.

### Prod findings that shape the design (read-only survey, 2026-09-13)
- **Cell sharing is real**: 41% of recent courses (349/852) share a cell across
  students; 26% engage the batch mechanism (`aa_cell_group >= 1`). → the engine MUST
  keep cells shareable. Dining is one-per-person.
- **"Reserved / keep aside" is mostly misused**: of 32 centres that fill it, only ~7
  actually benefit; 12 enter values entirely outside their own range (silently inert).
  → the new UI must *validate and surface* config, not just accept free text.

## 2. Goals / non-goals

**Goals**
- One shared engine for **cells + dining**, each **main + group-wise**.
- A **fully inline-editable "Review & Edit" workbench** per resource: see every
  attendee and value at once, edit inline, auto-fill non-fixed, save all together.
- Reliable, testable reserved/fixed handling; config validation.
- Phased rollout: dining ships first; cells migrate only after a proven parity diff
  against 800+ historical courses.

**Non-goals**
- The seating plan (separate feature) is untouched.
- No change to *who* is eligible (students-only today — see Open Questions).

## 3. Decisions locked (with the user, 2026-09-13)
1. Workbench is a **fully inline-editable grid** (not read-only + popup).
2. **Keep shareable cells** (batch mechanism); dining stays one-per-person.
3. **Full unified plan, phased** (P1–P4 below).
4. **Cells get group-wise ranges too** (parallel to dining).
5. **Eligibility: students always; servers OPTIONAL.** Students (`a_type='Student'`)
   are always allocated. Servers (`a_type='Sevak'`) are allocated only when the centre
   configures a **separate server range** — independent per resource (a centre can set
   server dining but not server cells). Servers draw from their own pool (no overlap
   with student seat numbers), and support **group-wise too** (parallel to students).
6. **Workbench is ONE page** with a Cell/Dining resource toggle (+ main/group-wise
   tabs), not two separate pages.
7. **Keep the existing surfaces as shortcuts**: the read-only printable list pages and
   the per-person Zero-Day popup stay; the workbench is the primary surface, not the only one.

## 4. Architecture

### 4.1 Resource descriptor
A small array describes each resource; the engine is generic over it.
```
CELL   = { name:'Cell',   has:'cs_has_cells',  config:'cs_cells_config',
           main_col:'aa_cell',   group_col:'aa_group_cell',   fixed_col:'aa_cell_fixed',
           batch_col:'aa_cell_group', shareable:true }
DINING = { name:'Dining', has:'cs_has_dining', config:'cs_dining_config',
           main_col:'aa_dining', group_col:'aa_group_dining', fixed_col:'aa_dining_fixed',
           batch_col:null,            shareable:false }
```

### 4.2 Engine (new `inc/allocate.inc`) — pure, unit-testable functions
- `dh_alloc_expand($str)` — "1-40, 55, A1-A5" → array of labels (letter-prefix aware).
- `dh_alloc_pool($ini, $section_key)` → `{sep, cells[], old[], new[], reserved[]}`.
- `dh_alloc_effective_section($ini, $gender, $group, $mode, $class)` — resolves the INI
  section for an attendee. `$class` ∈ {student, server}; base = `GENDER` + (`_SERVER`
  when server). Group mode → `base_n` if present, else `base`. Students always resolve;
  servers resolve only when their server section(s) exist, else the attendee is skipped.
- `dh_alloc_run($centre, $course, $desc, $mode)` — allocate + persist; returns summary.
- `dh_alloc_validate($ini, $attendee_counts)` → warnings (out-of-range reserved,
  pool-smaller-than-demand, duplicate literals).

### 4.3 Allocation semantics (precise, shared by both resources)
- **Order**: existing seniority/course order.
- **Old/new**: `sep` → separate Old/New pools; combined `Cells` → old filled first,
  then new, from one queue.
- **Reserved**: config `Reserved` **plus** every fixed non-empty value is removed from
  the pool. Out-of-range fixed values are kept as-is (never invented).
- **Group-wise**: per attendee, effective section = `[GENDER_n]` (their AT-group) else
  `[GENDER]`. Groups that fall back to the same default **share one queue** (no seat
  collisions); a group with its own range that runs out → unique: blank / shareable:
  next batch.
- **Class (student/server)**: servers use the `_SERVER` variant of every section
  (`[GENDER_SERVER]`, `[GENDER_SERVER_n]`) and their own pool, so server and student
  numbers never collide. Servers are included only when a server section exists for the
  resource; otherwise servers are left unallocated (current behaviour). Servers write
  the same columns as students (they are distinct people).
- **shareable=true (cells)**: when attendees > pool, cycle into batches; `batch_col`
  (aa_cell_group) records the batch index, so the same cell is reused across batches.
- **shareable=false (dining)**: pool exhausted → remaining attendees get blank.

### 4.4 Data model
- Add `aa_group_cell` varchar(10) to `dh_applicant_attended` (dining columns already
  exist: `aa_dining`, `aa_group_dining`, `aa_dining_fixed`).
- Cell config (`cs_cells_config`) gains `[MALE_n]`/`[FEMALE_n]` group sections — same
  INI grammar as dining.
- Keep `aa_cell_group` as the **batch index**; relabel as "batch" in code/UI to end
  the confusion with AT-group.
- No `.install` files exist here → `ALTER`s applied local-first, then prod at deploy
  (utf8mb4).

### 4.5 Centre-settings config UI
- Refactor the dining "defaults always shown + add-a-group-on-demand" form builder
  into a **shared helper** that generates both the cell and dining config fieldsets
  from the descriptor.
- Per resource the fieldset has: **Student** defaults (M/F, always shown) + Student
  groups (add-on-demand); and an optional **Servers** subsection — Server defaults
  (M/F) + Server groups (add-on-demand), all collapsed/off until the operator adds
  them. Empty server config ⇒ servers not allocated.
- INI grammar: `[MALE]`,`[FEMALE]`,`[MALE_n]`,`[FEMALE_n]` (students) plus
  `[MALE_SERVER]`,`[FEMALE_SERVER]`,`[MALE_SERVER_n]`,`[FEMALE_SERVER_n]` (servers);
  each section keeps the same keys (Cells | Old/New, Reserved).
- Keep the Reserved field, but add inline validation hinting (flag out-of-range).

### 4.6 The Review & Edit workbench (centrepiece)
- Routes: `cell-workbench/%centre_id/%course_id`, `dining-workbench/%centre_id/%course_id`
  (or one route with a `?res=` toggle). Access: `access zero day`.
- **Editable grid**: rows = attendees grouped by class (Students / Servers) → gender →
  old-new → AT-group; columns = **Main**, **Group-wise**, **Fixed** (+ **Batch** for
  cells). Server rows appear in their own sections and only when server allocation is
  configured. Inline-edit any value, tick Fixed.
- **Actions**: Auto-fill main · Auto-fill group-wise · Clear non-fixed · **Save all**
  (bulk AJAX, guarded by `dh_manageapp_lock_acquire`) · Print/export.
- **Live validation panel**: duplicate seats, out-of-range/reserved warnings,
  assigned-vs-pool counts, unassigned list.
- Auto-fill calls the engine; manual edits + Fixed persist through it. The current
  read-only list pages + per-person Zero-Day popup remain as optional shortcuts.

## 5. Phased rollout (each local-first; deploy only on explicit go-ahead)
- **P1** — Build engine; re-base the (unshipped) **dining** allocator onto it. Low risk.
- **P2** — Dining **workbench**; ship dining (+ its 5 schema ALTERs already staged).
- **P3** — Port **cells** onto the engine behind a compare harness: run the new engine
  over N historical courses and **diff vs stored `aa_cell`/batches**; investigate every
  diff (this also root-causes the Giri anomaly). Switch only when diffs are understood.
- **P4** — Cell **workbench**; retire `generate_cell_list()`.

## 6. Risks & mitigations
- *Changing live cell behaviour* → P3 parity diff across 800+ courses before switch.
- *Subtle batch semantics* → unit tests on the engine + the diff harness.
- *Bulk-save concurrency* → reuse existing `dh_manageapp_lock_acquire`.
- *Large centre form* → the add-group-on-demand pattern (already built for dining).

## 7. Open questions
All resolved 2026-09-13 — see §3 items 5–7 (students-only; one-page workbench with a
resource toggle; keep existing list pages + popup as shortcuts). No open questions remain.

## 8. Current state (already on local, not deployed)
Dining v1 is built + tested on local (separate allocator, config UI with add-on-demand
groups, Dining List + Group-wise Dining List pages, Zero-Day fix fields, teacher/manager
columns). Under this plan it becomes the **P1/P2** deliverable, re-based on the engine.
See memory `dining-seat-list`.
