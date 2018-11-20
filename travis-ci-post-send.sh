#!/bin/bash

case $1 in
  "success" )
    EMBED_COLOR=3066993
    STATUS_MESSAGE="Success"
    AVATAR="https://travis-ci.org/images/logos/TravisCI-Mascot-blue.png"
    ;;

  "failure" )
    EMBED_COLOR=15158332
    STATUS_MESSAGE="Failed"
    AVATAR="https://travis-ci.org/images/logos/TravisCI-Mascot-red.png"
    ;;

  * )
    EMBED_COLOR=0
    STATUS_MESSAGE="Unknown"
    AVATAR="https://travis-ci.org/images/logos/TravisCI-Mascot-1.png"
    ;;
esac

PREVIOUS_COMMIT="$(git rev-parse --short=12 "$TRAVIS_COMMIT"^1)"
if [ $PREVIOUS_COMMIT == ${TRAVIS_COMMIT_RANGE:0:12} ]; then
  COMMIT_URL='https://github.com/'"$TRAVIS_REPO_SLUG"'/commit/'"$TRAVIS_COMMIT"''
  COMMIT_RANGE=${TRAVIS_COMMIT:0:12}
else
  COMMIT_URL='https://github.com/'"$TRAVIS_REPO_SLUG"'/compare/'"$TRAVIS_COMMIT_RANGE"''
  COMMIT_RANGE=$TRAVIS_COMMIT_RANGE
fi

AUTHOR_LIST="$(git log "$TRAVIS_COMMIT_RANGE" --pretty="%aN, " | sort -u)"
AUTHOR_LIST=${AUTHOR_LIST:0:${#AUTHOR_LIST}-2}
COMMITTER_LIST="$(git log "$TRAVIS_COMMIT_RANGE" --pretty="%cN, " | sort -u)"
COMMITTER_LIST=${COMMITTER_LIST:0:${#COMMITTER_LIST}-2}
COMMIT_MESSAGE="$(git log "$TRAVIS_COMMIT_RANGE" --pretty="%B")"

CREDITS="by $AUTHOR_LIST"

if [[ $TRAVIS_PULL_REQUEST != false ]]; then
  URL="https://github.com/$TRAVIS_REPO_SLUG/pull/$TRAVIS_PULL_REQUEST"
else
  URL=""
fi

TIMESTAMP=$(date --utc +%FT%TZ)
case $1 in
  "success" )
    if [[ $UTESTS == true ]]; then
      UTESTS_TXT=$(<${UTESTS_RESULTS})  
      if [[ ${#UTESTS_TXT} > 1024 ]]; then
        UTESTS_TXT=${UTESTS_TXT:0:1024}
      fi
	   UTESTS_TXT=${UTESTS_TXT//\"/\'}
	   UTESTS_TXT=${UTESTS_TXT//'\n'/\\n}
      UNIT_TEST_RESULTS=', "fields": [{"name": "Unit Tests - Passed", "value": "'"$UTESTS_TXT"'" } ]'
    else 
      UNIT_TEST_RESULTS=""
    fi
    ;;
  "failure" )
    if [[ $UTESTS == true ]]; then
      UTESTS_TXT=$(<${UTESTS_RESULTS})  
      if [[ ${#UTESTS_TXT} > 1024 ]]; then
        UTESTS_TXT=${UTESTS_TXT:0:1024}
      fi
	   UTESTS_TXT=${UTESTS_TXT//\"/\'}
	   UTESTS_TXT=${UTESTS_TXT//'\n'/\\n}
      UNIT_TEST_RESULTS=', "fields": [{"name": "Unit Tests - Failed", "value": "'"$UTESTS_TXT"'" } ]'
    else 
      UNIT_TEST_RESULTS=""
    fi
    ;;
esac
	
WEBHOOK_DATA='{
  "avatar_url": "https://travis-ci.org/images/logos/TravisCI-Mascot-1.png",
  "embeds": 
  [{
    "color": '$EMBED_COLOR',
    "author": { "name": "Job #'"$TRAVIS_JOB_NUMBER"' '"$BUILD_NAME"'", "url": "'"$TRAVIS_BUILD_WEB_URL"'","icon_url": "'$AVATAR'" },
    "title": "'"$TRAVIS_BRANCH"' - '"$COMMIT_RANGE"'",
    "description": " - '"${COMMIT_MESSAGE//$'\n'/\\n - }"'",
    "timestamp": "'"$TIMESTAMP"'",
    "url": "'"$COMMIT_URL"'",
    "footer": { "text": "'"$CREDITS"'" }
    '$UNIT_TEST_RESULTS'
  }]}'
	
echo -e "$WEBHOOK_DATA"
(curl --connect-timeout 5 --max-time 10 --retry 5 --retry-delay 0 --retry-max-time 40 -H Content-Type:application/json -d "$WEBHOOK_DATA" "$2" \
&& echo -e "\\n[Webhook]: Successfully sent the webhook.") || echo -e "\\n[Webhook]: Unable to send webhook."