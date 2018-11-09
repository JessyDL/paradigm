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

AUTHOR_NAME="$(git log -1 "$TRAVIS_COMMIT" --pretty="%aN")"
COMMITTER_NAME="$(git log -1 "$TRAVIS_COMMIT" --pretty="%cN")"
COMMIT_MESSAGE="$(git log -1 "$TRAVIS_COMMIT" --pretty="%s%n%b")"

if [ "$AUTHOR_NAME" == "$COMMITTER_NAME" ]; then
  CREDITS="by $AUTHOR_NAME"
else
  CREDITS="by $AUTHOR_NAME & $COMMITTER_NAME"
fi

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
    "title": "'"$TRAVIS_BRANCH"' - '"${TRAVIS_COMMIT:0:7}"'",
    "description": "'"$COMMIT_MESSAGE"'",
    "timestamp": "'"$TIMESTAMP"'",
    "url": "https://github.com/'"$TRAVIS_REPO_SLUG"'/commit/'"$TRAVIS_COMMIT"'",
    "footer": "'"$CREDITS"'",
    '"$UNIT_TEST_RESULTS"'
  }]}'
	
echo -e "$WEBHOOK_DATA"
(curl --connect-timeout 5 --max-time 10 --retry 5 --retry-delay 0 --retry-max-time 40 -H Content-Type:application/json -d "$WEBHOOK_DATA" "$2" \
&& echo -e "\\n[Webhook]: Successfully sent the webhook.") || echo -e "\\n[Webhook]: Unable to send webhook."