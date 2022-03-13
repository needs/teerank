#
# Test image.
#

FROM python:3

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY shared/requirements.txt shared/requirements.txt
COPY backend/requirements.txt backend/requirements.txt
COPY frontend/requirements.txt frontend/requirements.txt
COPY test/requirements.txt test/requirements.txt

RUN pip install --no-cache-dir \
	-r shared/requirements.txt \
	-r backend/requirements.txt \
	-r frontend/requirements.txt \
	-r test/requirements.txt

COPY shared shared
COPY backend backend
COPY frontend frontend
COPY test test

CMD [ "ptw", "--poll", "--runner", "pytest --cov --cov-report term-missing --pylint --pylint-rcfile=test/pylint.rc -W ignore::DeprecationWarning" ]
