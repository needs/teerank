#!/bin/bash

set -e
trap "rm $TEERANK_DB*" EXIT
./teerank-create-test-database

# We expect success for those pages
urls=(
	"players"
	"clans"
	"servers"

	"players?gametype=DM"
	"clans?gametype=DM"
	"servers?gametype=DM"

	"gametypes"
	"maps"
	"maps?gametype=DM"

	"player?name=tee1"
	"player/historic.svg?name=tee1"
	"clan?name=clan1"
	"server?ip=1.1.1.1&port=8000"

	"search?q=tee"
	"search/players?q=tee"
	"search/clans?q=clan"
	"search/servers?q=server"

	"about"
	"status"

	"players.json"
	"clans.json"
	"servers.json"

	"players.json?gametype=DM"
	"clans.json?gametype=DM"
	"servers.json?gametype=DM"

	"player.json?name=tee1"
	"clan.json?name=PAL"
	"server.json?ip=1.1.1.1&port=8000"

	"about.json"
)

for url in ${urls[@]}; do
	surl=(${url//'?'/ })
	export DOCUMENT_URI="${surl[0]}"
	export QUERY_STRING="${surl[1]}"

	if ! ./teerank.cgi >/dev/null; then
		echo "Expected success for page: '$url'"
		exit 1
	fi
done
