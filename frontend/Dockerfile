#
# Frontend image.
#

FROM python:3

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
