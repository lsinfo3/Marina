import argparse
import json
import pathlib


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('src', type=pathlib.Path, help='directory of context.json')
    parser.add_argument('dst', type=pathlib.Path, help='directory for header files')
    args = parser.parse_args()

    with (args.src / 'context.json').open() as f:
        data = json.load(f)
    tables = data['tables']
    handles = []

    for table in tables:
        name = table['name']
        if name.endswith('reg'):
            print(f"extracting handle for table {name}")
            handles.append((name, table['handle'], table['direction']))

    args.dst.mkdir(exist_ok=True)
    with (args.dst / 'table_handles.h').open('w') as f:
        f.write("/* Generated! Do not modify*/\n\n")
        for name, handle, direction in handles:
            f.write(f"#define {name}_handle {handle}\n")
            f.write(f"#define {name}_on_egress {int(direction == 'egress')}\n")


if __name__ == '__main__':
    main()
