#
# Production image.
#

FROM python:3 AS production

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY backend/requirements.txt backend/requirements.txt
RUN pip install --no-cache-dir -r backend/requirements.txt
COPY backend backend

CMD [ "python", "backend/main.py" ]

#
# Test image.
#

FROM production AS test

COPY backend-test/requirements.txt backend-test/requirements.txt
RUN pip install --no-cache-dir -r backend-test/requirements.txt
COPY backend-test backend-test

CMD [ "pytest", "--cov", "--cov-report", "term-missing", "--pylint" ]
