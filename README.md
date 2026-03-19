# Spotify Playlist Generators

## Python Environment (uv)

This project uses **uv** as the Python package manager and runner.

### Install dependencies

```bash
uv add spotipy python-dotenv
```

### Run scripts

Use `uv run` to execute Python scripts in the project environment:

```bash
uv run python python/create_playlist.py <path_to_jsonl> --name "Playlist Name"
```

**Notes:**

- `uv` creates and manages a virtual environment automatically  
- No need to manually activate a venv  
- `pyproject.toml` and `uv.lock` define the environment  

---

## Overview

This repository contains multiple **playlist generators** based on Spotify data.

Each generator is an independent program that:

1. Takes input data (e.g. streaming history, seeds, config)
2. Applies an algorithm
3. Outputs a JSONL file of track candidates
4. Can be used with the Python uploader to create a Spotify playlist

---

## Data Flow

```text
Input data → C++ generator → JSONL → Python → Spotify playlist
```

---

## Input Data

Generators may use different types of input:

- **Spotify streaming history**

  ```text
  data/raw/<user>/
  ```

- **Preprocessed data**

  ```text
  data/processed/
  ```

- **Custom inputs** (e.g. seeds, configs)

---

## Output Format (JSONL)

All generators must output a JSONL file:

```text
data/output/<name>.jsonl
```

Each line:

```json
{"uri":"spotify:track:..."}
```

---

## Running Generators

Each generator defines its own interface.

### Example

```bash
./build/spotify_timemachine <user>
```

---

## Uploading to Spotify

```bash
uv run python python/create_playlist.py <path_to_jsonl> --name "Playlist Name"
```

### Example

```bash
uv run python python/create_playlist.py data/output/art/spotify_timemachine.jsonl --name "Time Machine"
```

---

## Generator Design Principles

- Generators are **independent tools**
- Input/output is **file-based**
- Output format is **standardized (JSONL)**
- No generator depends on another

---

## Example Generators

### spotify_timemachine

**Input:**
- user streaming history

**Output:**
- tracks that dominated listening in time windows

---

## Adding a New Generator

1. Create:

```text
cpp/generators/<name>.cpp
```

2. Define its inputs (arguments, files, etc.)

3. Output:

```text
data/output/<name>.jsonl
```

4. Ensure JSONL format is valid