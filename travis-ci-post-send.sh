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
WEBHOOK_DATA='{
  "username": "",
  "avatar_url": "https://travis-ci.org/images/logos/TravisCI-Mascot-1.png",
  "embeds": [ {
    "color": '$EMBED_COLOR',
    "author": {
      "name": "Job #'"$TRAVIS_JOB_NUMBER"' '"$BUILD_NAME"'",
      "url": "'"$TRAVIS_BUILD_WEB_URL"'",
      "icon_url": "'$AVATAR'"
    },
    "title": "'"$STATUS_MESSAGE"'",
    "description": "'"${COMMIT_MESSAGE//$'\n'/ }"\\n"$CREDITS"'",
    "url": "'"$URL"'",
    "fields": [
      {
        "name": "Commit",
        "value": "'"[\`${TRAVIS_COMMIT:0:7}\`](https://github.com/$TRAVIS_REPO_SLUG/commit/$TRAVIS_COMMIT)"'",
        "inline": true
      },
      {
        "name": "Branch/Tag",
        "value": "'"[\`$TRAVIS_BRANCH\`](https://github.com/$TRAVIS_REPO_SLUG/tree/$TRAVIS_BRANCH)"'",
        "inline": true
      }
    ],
    "timestamp": "'"$TIMESTAMP"'"
  } ]
}'
echo -e "$WEBHOOK_DATA"
(curl --connect-timeout 5 --max-time 10 --retry 5 --retry-delay 0 --retry-max-time 40 -H Content-Type:application/json -d "$WEBHOOK_DATA" "$2" \
&& echo -e "\\n[Webhook]: Successfully sent the webhook.") || echo -e "\\n[Webhook]: Unable to send webhook."