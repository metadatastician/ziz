# Handover prompt: verify the merged ziz repair and finish the outstanding work

Paste the prompt below into the next Agent Mode session. Re-check live state first; this file is a snapshot, not authority.

---

You are taking over work concerning `metadatastician/ziz` and several estate follow-ups. Work only on your Arena-assigned branch; do not switch branches. The user's request is: verify that their actions actually resolved the issues, finish anything you have permission to do, and report evidence rather than assumptions.

## Ground truth captured 2026-09-28 UTC

- PR [metadatastician/ziz#2](https://github.com/metadatastician/ziz/pull/2) **is MERGED** at `2026-09-28T09:48:09Z`, merge commit `0564622e71348c60d884b732a9cfb7593525ae2a`, based on `76ea583...`. It includes two changes: CodeQL pin+lock/ruleset recipe repair, and a fail-closed Dependabot auto-merge gate.
- The live `main` has the CodeQL init/analyze pins at `b96794f015dfd88f77b49b1c93e0fa7110f94c63` (`v4.38.0`), matching the corresponding `actions.lock` entry. The `Lock Sync Gate` run on the merge push succeeded.
- The merged Dependabot workflow gates on GHSA + patch/minor + CVSS LOW/MODERATE; HIGH/CRITICAL, major, routine, or unreadable CVSS are skipped. Its comments correctly acknowledge that an empty required-status-check list is not a meaningful safety net. Live `EstateBranching` ruleset exists and its `required_status_checks` rule is absent; PR approval count is 0. Do not restore an unconditional merge branch unless the owner explicitly decides to reverse policy and fixes the protection premise.
- **The CodeQL problem is not proven resolved.** The `CodeQL Security Analysis` PR run for head `63ab853...` is `startup_failure` (run `36405365437`). The post-merge run on `main` `0564622...` ran from 09:48:12 to 09:51:08 UTC, then failed at `Perform CodeQL Analysis` (run `36405799171`). The GitHub code-scanning analyses API returns 403: “Code Security must be enabled for this repository to use code scanning.” A later PR #3 CodeQL run (run `36409629916`) was again an immediate zero-job startup failure attributed to `arena-ai-coding-agent[bot]`. The cause of the main-run analysis failure was not available from downloaded logs; do not claim the 403 definitively caused it without owner-side log inspection. The live `EstateBranching` still requires CodeQL results. See `docs/CI-OPERATIONS-RUNBOOK.adoc` for the measured diagnosis and owner actions.
- PR #3 is documentation-only as originally opened, and remains blocked. Its current branch has been extended with a CodeQL coverage matrix for Actions + JavaScript/TypeScript + Rust, because those are present in the repository; it also adds an operations runbook and corrects stale Actions-billing comments. A CodeQL upload/security setting still requires owner action and may keep PR #3 blocked.
- Four requested MetaManifold-WebUI refs still existed when checked: `arena/01a0db23-metamanifold-webui`, `arena/01a0de46-metamanifold-webui`, `arena/01a0df2a-metamanifold-webui`, `arena/01a0df7c-metamanifold-webui`. Compare against `main` returned no changed files for each (first diverged with 4 commits ahead/3 behind, remaining three were behind only). Deletion attempts for all four returned HTTP 403 `Resource not accessible by integration`. Re-check before retrying; only delete after confirming the refs remain content-identical and are not backing open PRs. If still 403, give the owner the exact safe deletion commands; never claim deletion succeeded.
- `hyperpolymath/standards#1005` remains open with **3 comments**, all from `hyperpolymath`; the requested new AC2 decay comment was not present at last check. The previous session's `PENDING-comment-standards-1005.md` is not in this checkout. Re-measure the stated 28-file regression (blocked v4.38.1 target with misleading v4.38.0 comments), prepare/recover the intended comment text, and post only if authorized and supported by current evidence. Verify the resulting comment by URL/body, not only count.
- `hypatia#868` (proposed GS009 merge-base rule), `rsr-template-repo#204` (automerge policy defect), and `wordpress-tools#96` (bot-triggered Actions startup failures) were still open with no comments by this agent at last check. Review before deciding whether evidence merits adding a comment/issue.
- `ziz/.github/CODEOWNERS` contains `* @{{OWNER}}` and similar entries. `{{OWNER}}` is not a real GitHub user, but determine how GitHub treats invalid code-owner patterns before claiming this requirement is either inert or unsatisfiable. The live ruleset requires code-owner review and extra approval for unattributed changes.

## First actions: refresh facts

Use authenticated `gh` and the current repo. Start read-only:

```sh
gh pr view 2 -R metadatastician/ziz --json state,mergedAt,mergeCommit,headRefOid,url
gh run list -R metadatastician/ziz --workflow 'CodeQL Security Analysis' --limit 20 \
  --json databaseId,event,conclusion,headSha,headBranch,createdAt,url
gh run view 36405799171 -R metadatastician/ziz --log-failed
gh api repos/metadatastician/ziz/code-scanning/analyses
gh api repos/metadatastician/ziz/rulesets/18225024
gh api repos/hyperpolymath/MetaManifold-WebUI/branches --paginate --jq '.[].name'
gh issue view 1005 -R hyperpolymath/standards --json state,comments,url
```

If IDs changed, discover by ruleset name rather than assuming the numeric ID. Inspect failed run jobs/logs; an Actions `failure` is not itself proof of a security finding. Separate runner/startup failures, workflow/SARIF errors, and actual alerts.

## Resolve what is actionable

1. **CodeQL gate / verification:** Find why the main push run failed and whether Code Security is enabled or can be enabled. Do not disable code scanning, weaken the ruleset, or use an admin bypass as a shortcut. If the org-side bot-trigger issue remains, distinguish it from a lock/YAML desync; a human rerun/reopen only helps if evidence shows that event actor is the cause. Report the smallest valid owner action if the integration cannot fix it.
2. **Four MetaManifold branches:** Retry the branch deletions only if current comparisons still show no file delta and no live PR dependency. The previous integration received 403, so likely this needs a human with write access. Safe command pattern after revalidation:
   ```sh
   gh api -X DELETE repos/hyperpolymath/MetaManifold-WebUI/git/refs/heads/BRANCH
   ```
   Verify each is absent from the branch list afterwards.
3. **standards#1005:** Re-run the estate census; classify by actual resolved commit/tag, not comments alone. Confirm exactly which files/repositories regressed and that they are active workflows. Then post the prepared factual comment if permitted, or give the owner a paste-ready comment. Verify it appears on the issue.
4. **GS009:** Investigate `hypatia#868`. The rule must test for a shared merge base with the repository default branch, not ancestry: squash merges mean ancestry incorrectly flags merged branches. Provide a tested implementation/fixture or a concise evidence-backed comment if authorized.
5. **CODEOWNERS:** Verify the invalid `@{{OWNER}}` template issue, its estate scope, and actual GitHub rule semantics. File or comment in the right template issue only if supported; avoid asserting “53 repos” without remeasurement.
6. Check the relevant issue/PR state before creating duplicates. Keep any additional repo changes scoped and tested.

## Acceptance criteria / report back

Report each item `RESOLVED`, `PARTIAL`, `BLOCKED`, or `NOT VERIFIED`, with URLs, SHAs, run IDs, and command evidence:

- PR #2 merged and expected files/rules landed.
- Dependabot gate really skips HIGH/CRITICAL, major, routine, and unknown CVSS (the merge alone does not prove runtime behavior); the source and existing tests should agree.
- CodeQL produces a successful PR result and a code-scanning analysis on the intended commit, or a precise owner-level blocker is identified. The old PR's `startup_failure` and failed main push are not success.
- Four branches deleted, or explicitly blocked with 403 and owner commands.
- standards#1005 regression re-measured and comment posted/ready for owner; verify comment URL.
- GS009 merge-base criterion validated against normal, squash-merged, live-unmerged, and content-identical branches.
- CODEOWNERS finding validated without overstating semantics or scope.

Evidence wins over this snapshot. Do not claim a user action worked merely because PR #2 merged. Do not force-push, delete refs backing open PRs, edit organization rules without authorization, or conflate a clean file diff with ancestry.
