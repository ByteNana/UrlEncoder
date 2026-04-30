#!/usr/bin/env bash
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$BRANCH" = "master" ]; then
  echo "  ERROR: Direct pushes to 'master' are not allowed. Use a PR with squash-merge instead."
  exit 1
fi
if [[ "$BRANCH" == rc-* ]]; then
  ZERO="0000000000000000000000000000000000000000"
  while read -r local_ref local_oid remote_ref remote_oid; do
    [ "$local_oid" = "$ZERO" ] && continue
    [ "$remote_oid" = "$ZERO" ] && continue
    if ! git merge-base --is-ancestor "$remote_oid" "$local_oid" 2>/dev/null; then
      echo "  ERROR: Force push to '${BRANCH}' is not allowed."
      exit 1
    fi
  done
fi
