#!/bin/bash
QUERY=$(cat << EOF
{"query": "query TournamentsByState { tournaments(query: { perPage: 100, filter: {addrState: \"AB\", afterDate: $(date +%s), videogameIds: [1]}}) \
{ nodes { name url city startAt venueAddress } } }"}
EOF)

curl -X POST https://api.start.gg/gql/alpha \
    -H 'Content-Type: application/json' \
    -H 'Authorization: Bearer 307cd73a7f0bef58d713214da4f9b47c' \
    -d "$QUERY" \
    > ab_tournaments.json

build/fill_template
