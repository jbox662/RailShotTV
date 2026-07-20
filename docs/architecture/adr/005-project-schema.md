# ADR-005: Project schema v1

## Status
Accepted

## Context
React prototype stored fragmented localStorage blobs.

## Decision
Single versioned JSON document (`schemaVersion: 1`) owned by `Project`, with
secrets referenced by Credential Manager IDs only.

## Consequences
Migrations go through `JsonSchema::migrateProject`. Autosave writes `.bak` first.
