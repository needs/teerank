#
# Backend image
#

FROM python:3 AS production

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY backend/requirements.txt backend/requirements.txt
RUN pip install --no-cache-dir -r backend/requirements.txt

COPY backend backend

EXPOSE 8311/udp

CMD [ "python", "backend/main.py" ]

# Restart the backend as soon as a file changed.  For this to work with a
# development workflow, host source files needs to be binded into the container
# using binded volumes, replacing files that were copied previously by the COPY
# command.
#
# The neat command pytest-watch just do that, and thanks to the --runner
# parameter, we can launch our own command instead of the pytest command.  here
# with use it to launch the server.
#
# The --poll parameter is necessary because otherwise pytest-watch don't work
# with binded volumes.

FROM production AS development

RUN pip install --no-cache-dir pytest-watch
CMD [ "pytest-watch", "--poll", "--runner", "python backend/main.py"]
