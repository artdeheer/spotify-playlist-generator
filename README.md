# Spotify Playlist Generator Basics

## Files
- `.env` — Spotify credentials
- `python/create_playlist.py` — creates a Spotify playlist from a JSONL file
- `data/output/` — put your generator output files here

## Install
```bash
pip install -r requirements.txt
```

## .env
Fill in:
- `SPOTIFY_CLIENT_ID`
- `SPOTIFY_CLIENT_SECRET`
- `SPOTIFY_REDIRECT_URI`
- `SPOTIFY_USER_ID`

## Expected JSONL format
One JSON object per line, for example:
```json
{"uri":"spotify:track:4uLU6hMCjMI75M1A2tKUQC"}
{"id":"1301WleyT98MSxVHPZCA6M"}
{"track_id":"3n3Ppam7vgaVa1iaRUc9Lp"}
```

## Run
```bash
python python/create_playlist.py data/output/mood.jsonl --name "Mood Playlist"
```

For a public playlist:
```bash
python python/create_playlist.py data/output/mood.jsonl --name "Mood Playlist" --public
```