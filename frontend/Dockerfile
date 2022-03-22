#
# Frontend image.
#

FROM python:3 AS production

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY frontend/requirements.txt frontend/requirements.txt
RUN pip install --no-cache-dir -r frontend/requirements.txt

COPY frontend frontend

EXPOSE 8080

CMD [ "waitress-serve", "frontend.app:create_app" ]

# The development target launch the flask server instead of a production server.
# The flask server reloads the server when source file changes.  Of course for
# this to work source folders needs to be overided with host source folders
# using a binded volume when launching the container.

FROM production AS development

ENV FLASK_APP "frontend.app:create_app"
ENV FLASK_ENV "development"

CMD [ "flask", "run", "--host", "0.0.0.0" ]
