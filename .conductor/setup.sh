#!/usr/bin/env bash
# Conductor setup script: make lint, build and test work in the workspace.  The
# app itself runs from the repository root, see dev.sh.
set -euo pipefail

cd "$(dirname "$0")/.."
set -a
. .conductor/dev.env
set +a

export COMPOSE_PROJECT_NAME=teerank

npm install
# Only postgres: generate --sql compiles the typed queries against the database.
docker compose up --detach --wait postgres
npx prisma migrate deploy --schema=libs/prisma/prisma/schema.prisma
npx prisma generate --sql --schema=libs/prisma/prisma/schema.prisma
