# Database connection budget

Postgres (`teerankio-postgres2`) has `max_connections = 500` (3 reserved, so 497 usable).
Every app connects directly with Prisma; there is no pooler in front (`teerankio-pgbouncer`
exists but is suspended — the stray lowercase `database_url` secret that pointed at it was
removed from the scheduler in Aug 2026).

Prisma opens up to `connection_limit` connections **per process** and never gives them back
below the high-water mark, so the budget is `machines × connection_limit` per app. The limit
is set as a query param on each app's `DATABASE_URL` Fly secret:

| App                   | Machines (max) | `connection_limit` | Worst case |
|-----------------------|----------------|--------------------|------------|
| `teerankio-frontend`  | 4              | 20                 | 80         |
| `teerankio-worker`    | 3              | 30                 | 90         |
| `teerankio-scheduler` | 1              | 10                 | 10         |
| Total                 |                |                    | **180**    |

Two effects inflate the count beyond the table, so keep large headroom under 497:

- The frontend uses `auto_stop_machines`; a stopping Fly machine is killed without closing
  its TCP connections, so its backends linger as `idle` on Postgres until haproxy (which
  fronts Postgres on the machine) reaps them, typically within ~30 minutes. Each frontend
  flap can therefore strand up to one pool's worth of connections.
- `pg_stat_activity.client_addr` is useless for attribution — haproxy makes every client
  appear as the Postgres machine's own 6PN address. To attribute connections, run
  `ss -tn sport = :5432` on the Postgres machine and map peer addresses with
  `fly ips private -a <app>`.

## Incident 2026-08 (for context)

All three apps shared one `DATABASE_URL` secret with `connection_limit=128`, a ceiling of
~1024 across 8 processes. Connections hit 484/497 (474 idle), and jobs across the worker
fleet failed with `too many clients`, `Server has closed the connection`, and unreachable-DB
errors. Fixed 2026-08-11 by setting the per-app limits above. If a new app needs the
database, give it an explicit `connection_limit` and update this table.
