# This file is generated by Nx.
#
# Build the docker image with `npx nx docker-build worker`.
# Tip: Modify "docker-build" options in project.json to change docker build args.
#
# Run the container with `docker run -p 3000:3000 -t worker`.
FROM docker.io/node:lts-alpine

ENV HOST=0.0.0.0
ENV PORT=3000

WORKDIR /app

RUN addgroup --system worker && \
          adduser --system -G worker worker

COPY dist/apps/worker .
RUN chown -R worker:worker .

# You can remove this install step if you build with `--bundle` option.
# The bundled output will include external dependencies.
RUN npm --omit=dev -f install

COPY libs/prisma/prisma/schema.prisma prisma/schema.prisma
RUN npx prisma generate

CMD [ "node", "." ]
