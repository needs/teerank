SELECT pg_advisory_xact_lock(hashtext('rollup-write')) IS NULL AS locked;
