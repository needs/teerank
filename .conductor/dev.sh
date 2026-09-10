#!/usr/bin/env bash
# Conductor run script: docker, worker, scheduler and frontend, from the
# repository root that spotlight testing keeps in sync with the active workspace.
set -euo pipefail

cd "$(dirname "$0")/.."
set -a
. .conductor/dev.env
set +a
cd "${CONDUCTOR_ROOT_PATH:-.}"

export COMPOSE_PROJECT_NAME=teerank

[ -d node_modules ] || npm install

# Wait only for what the apps need; bullboard and pghero can take their time.
docker compose up --detach --wait postgres redis minio
docker compose up --detach

npx prisma migrate deploy --schema=libs/prisma/prisma/schema.prisma
npx prisma generate --sql --schema=libs/prisma/prisma/schema.prisma

# exec so a stop signal reaches nx itself rather than this wrapper shell.
exec node_modules/.bin/nx run-many -t serve -p frontend worker scheduler --output-style=stream
