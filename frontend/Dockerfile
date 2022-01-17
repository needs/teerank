#
# Frontend image.
#

FROM python:3 AS production

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY shared/requirements.txt shared/requirements.txt
COPY frontend/requirements.txt frontend/requirements.txt

RUN pip install --no-cache-dir \
	-r shared/requirements.txt \
	-r frontend/requirements.txt

COPY shared shared
COPY frontend frontend

EXPOSE 8080

CMD [ "waitress-serve", "frontend.main:app" ]

# The development target launch the flask server instead of a production server.
# The flask server reloads the server when source file changes.  Of course for
# this to work source folders needs to be overided with host source folders
# using a binded volume when launching the container.

FROM production AS development

ENV FLASK_APP "frontend.main:app"
ENV FLASK_ENV "development"

CMD [ "flask", "run", "--host", "0.0.0.0" ]
