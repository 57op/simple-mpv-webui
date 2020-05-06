#!/usr/bin/env python3

import subprocess
import re

from pathlib import Path
from typing import List, Tuple

REGEX_PROP_CMD = re.compile(r'(command|get_property)\s*\(\s*([\'`])(.+?)\2\s*\)')

def hash_k33(s: str, b: int) -> int:
  # invoking a c compiled program because i was having some mismatches
  # i will inspect this later on, not a priority
  p = subprocess.Popen(('utilities/hash', str(b), s), stdout=subprocess.PIPE)
  return int(p.stdout.read().decode().strip())

def find_props_cmds(file: Path) -> List[Tuple[str, str]]:
  matches = []

  with open(file, 'r') as fh:
    for line in fh:
      for m in REGEX_PROP_CMD.finditer(line):
        matches.append((m.group(1), m.group(3)))

  return matches

def to_c_array(hashmap: List[List[str]], name: str) -> str:
  max_bucket_len = max(map(lambda bucket: len(bucket), hashmap))
  c = []

  for bucket in hashmap:
    bucket = list(map(lambda key: f'"{key}"', bucket))

    if len(bucket) < max_bucket_len:
      bucket.extend(['NULL'] * (max_bucket_len - len(bucket)))

    c.append(f'  {{ {", ".join(bucket)} }}')

  c = ',\n'.join(c)

  return f'const char *{name}[{len(hashmap)}][{max_bucket_len}] = {{\n{c}\n}};'

def command_cut(cmd):
  cmd = cmd.split()
  cut = None

  for (i, part) in enumerate(cmd):
    if part[0:2] == '${':
      cut = i
      break
    else:
      try:
        float(part)
        cut = i
      except ValueError:
        pass

  if cut is not None:
    cmd = cmd[:cut]

  return ' '.join(cmd)

if __name__ == '__main__':
  import argparse

  def ext_path(p):
    p = Path(p)

    if not p.is_file():
      raise argparse.ArgumentTypeError(f'no such file `{p}`')

    return p

  parser = argparse.ArgumentParser()
  parser.add_argument('files', nargs='+', type=ext_path)
  parser.add_argument('--buckets', type=int, default=5)

  args = parser.parse_args()
  props = set()
  cmds = set()

  for file in args.files:
    for (fn, arg) in find_props_cmds(file):
      if fn == 'get_property':
        props.add(arg)
      else:
        cmds.add(command_cut(arg))

  max_prop_len = max(map(lambda prop: len(prop), props))
  max_cmd_len = max(map(lambda cmd: len(cmd), cmds))

  # build a static hashmap for properties
  buckets = args.buckets
  hashmap = [[] for i in range(buckets)]

  for prop in props:
    hashmap[hash_k33(prop, buckets)].append(prop)

  print(to_c_array(hashmap, 'props_hashmap'))

  # build a static hashmap for commands
  hashmap = [[] for i in range(buckets)]

  for cmd in cmds:
    hashmap[hash_k33(cmd, buckets)].append(cmd)

  print(to_c_array(hashmap, 'cmds_hashmap'))
