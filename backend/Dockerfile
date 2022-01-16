#
# Backend image
#

FROM python:3

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY shared/requirements.txt shared/requirements.txt
COPY backend/requirements.txt backend/requirements.txt

RUN pip install --no-cache-dir \
	-r shared/requirements.txt \
	-r backend/requirements.txt

COPY shared shared
COPY backend backend

EXPOSE 8311/udp

CMD [ "python", "backend/main.py" ]
