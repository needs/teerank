#
# Test image.
#

FROM python:3

WORKDIR /usr/src/app
ENV PYTHONPATH "${PYTHONPATH}:/usr/src/app"

COPY backend/requirements.txt backend/requirements.txt
COPY frontend/requirements.txt frontend/requirements.txt
COPY shared/requirements.txt shared/requirements.txt
COPY test/requirements.txt test/requirements.txt

RUN pip install --no-cache-dir \
	-r backend/requirements.txt \
	-r frontend/requirements.txt \
	-r shared/requirements.txt \
	-r test/requirements.txt

COPY backend backend
COPY frontend frontend
COPY test test

CMD [ "ptw", "--poll", "--runner", "pytest --cov --cov-report term-missing --pylint --pylint-rcfile=test/pylint.rc -W ignore::DeprecationWarning" ]
