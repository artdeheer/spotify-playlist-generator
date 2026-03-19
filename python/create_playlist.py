#!/usr/bin/env python3
"""
Create a Spotify playlist from a JSONL file in data/output/.

Expected JSONL format: one JSON object per line.
Each line should contain either:
- {"uri": "spotify:track:..."}
- {"id": "spotify_track_id"}
- {"track_id": "spotify_track_id"}

Usage:
    python python/create_playlist.py data/output/mood.jsonl --name "My Mood Playlist"

Setup:
    1. Put your credentials in .env
    2. pip install spotipy python-dotenv
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import List

from dotenv import load_dotenv
import spotipy
from spotipy.oauth2 import SpotifyOAuth


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a Spotify playlist from a JSONL file."
    )
    parser.add_argument(
        "jsonl_file",
        help="Path to the JSONL file, for example data/output/mood.jsonl",
    )
    parser.add_argument(
        "--name",
        required=True,
        help="Spotify playlist name",
    )
    parser.add_argument(
        "--description",
        default="Created automatically from a JSONL file.",
        help="Spotify playlist description",
    )
    parser.add_argument(
        "--public",
        action="store_true",
        help="Create playlist as public. Default is private.",
    )
    return parser.parse_args()


def load_required_env() -> dict:
    load_dotenv()
    keys = [
        "SPOTIFY_CLIENT_ID",
        "SPOTIFY_CLIENT_SECRET",
        "SPOTIFY_REDIRECT_URI",
        "SPOTIFY_USER_ID",
    ]
    values = {key: os.getenv(key, "").strip() for key in keys}
    missing = [key for key, value in values.items() if not value]
    if missing:
        raise RuntimeError(
            "Missing environment variables in .env: " + ", ".join(missing)
        )
    return values


def track_uri_from_record(record: dict) -> str:
    if "uri" in record and record["uri"]:
        return record["uri"]

    for key in ("id", "track_id"):
        if key in record and record[key]:
            return f"spotify:track:{record[key]}"

    raise ValueError(
        "Each JSONL line must include one of: uri, id, track_id"
    )


def read_track_uris(jsonl_path: Path) -> List[str]:
    if not jsonl_path.exists():
        raise FileNotFoundError(f"JSONL file not found: {jsonl_path}")

    uris: List[str] = []
    seen = set()

    with jsonl_path.open("r", encoding="utf-8") as f:
        for line_number, raw_line in enumerate(f, start=1):
            line = raw_line.strip()
            if not line:
                continue

            try:
                record = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(
                    f"Invalid JSON on line {line_number}: {exc}"
                ) from exc

            uri = track_uri_from_record(record)
            if uri not in seen:
                seen.add(uri)
                uris.append(uri)

    if not uris:
        raise ValueError("No track URIs found in the JSONL file.")

    return uris


def add_tracks_in_batches(sp: spotipy.Spotify, playlist_id: str, uris: List[str]) -> None:
    batch_size = 100
    for i in range(0, len(uris), batch_size):
        sp.playlist_add_items(playlist_id, uris[i:i + batch_size])


def main() -> None:
    args = parse_args()
    env = load_required_env()

    sp = spotipy.Spotify(
        auth_manager=SpotifyOAuth(
            client_id=env["SPOTIFY_CLIENT_ID"],
            client_secret=env["SPOTIFY_CLIENT_SECRET"],
            redirect_uri=env["SPOTIFY_REDIRECT_URI"],
            scope="playlist-modify-private playlist-modify-public",
            open_browser=True,
        )
    )

    track_uris = read_track_uris(Path(args.jsonl_file))

    playlist = sp.user_playlist_create(
        user=env["SPOTIFY_USER_ID"],
        name=args.name,
        public=args.public,
        description=args.description,
    )

    add_tracks_in_batches(sp, playlist["id"], track_uris)

    print("Playlist created successfully.")
    print(f"Name: {playlist['name']}")
    print(f"Tracks added: {len(track_uris)}")
    print(f"URL: {playlist['external_urls']['spotify']}")


if __name__ == "__main__":
    main()