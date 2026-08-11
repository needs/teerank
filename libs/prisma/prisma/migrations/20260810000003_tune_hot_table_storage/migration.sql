-- GameServerState rows are now updated in place on every poll; fillfactor < 100
-- leaves page space so those updates stay HOT (no index writes, dead tuples
-- reclaimed without autovacuum). Storage parameters are catalog-only changes:
-- existing pages are untouched and gain free space through normal HOT pruning.
ALTER TABLE "GameServerState" SET (fillfactor = 70);
ALTER TABLE "GameServerStateClient" SET (fillfactor = 70);

-- The snapshot archiver deletes in batches; more eager autovacuum keeps dead
-- tuples from accumulating faster than they're reclaimed during the backlog
-- drain. Harmless in steady state.
ALTER TABLE "GameServerClient" SET (autovacuum_vacuum_scale_factor = 0.01, autovacuum_vacuum_cost_limit = 2000);
ALTER TABLE "GameServerSnapshot" SET (autovacuum_vacuum_scale_factor = 0.01, autovacuum_vacuum_cost_limit = 2000);
