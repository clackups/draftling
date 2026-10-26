"""Draftling extension for idf.py: the --lang-pack option.

A language pack is the set of keyboard layouts, input methods and font
subsets built into the firmware (see the "Language pack" choice in
components/kb_layout/Kconfig.projbuild). Together with a board preset:

    idf.py --preset waveshare_rlcd42 --lang-pack japanese build

or, equivalently, with the environment variable:

    DRAFTLING_LANG_PACK=japanese idf.py --preset waveshare_rlcd42 build

For any pack other than "default" this
  * moves the build directory from build/<board> to
    build/<pack>/<board> (and its sdkconfig with it), so the images of
    different packs never share a build tree;
  * appends sdkconfig.defaults.lang_<pack> to the preset's
    SDKCONFIG_DEFAULTS;
  * passes -DDRAFTLING_LANG_PACK=<pack>, which CMakeLists.txt checks
    against the sdkconfig.

The default pack keeps the plain build/<board> directories.
"""

import json
import os
from typing import Any

LANG_PACKS = ('default', 'vietnamese', 'korean', 'japanese', 'chinese')
ENV_VAR = 'DRAFTLING_LANG_PACK'


def _preset_cache_variable(project_dir: str, preset: str, name: str) -> str | None:
    path = os.path.join(project_dir, 'CMakePresets.json')
    try:
        with open(path) as f:
            presets = json.load(f).get('configurePresets', [])
    except (OSError, ValueError):
        return None
    for p in presets:
        if p.get('name') == preset:
            return p.get('cacheVariables', {}).get(name)
    return None


def action_extensions(base_actions: dict, project_path: str) -> dict:
    def lang_pack_callback(ctx: Any, global_args: Any, tasks: list) -> None:
        pack = global_args.get('lang_pack') or 'default'
        if pack not in LANG_PACKS:
            from idf_py_actions.errors import FatalError
            raise FatalError(f'Unknown language pack "{pack}"; choose one of: {", ".join(LANG_PACKS)}')

        defines = global_args.define_cache_entry
        defines.append(f'DRAFTLING_LANG_PACK={pack}')
        if pack == 'default':
            return

        project_dir = os.path.realpath(global_args.project_dir)
        build_root = os.path.join(project_dir, 'build')
        build_dir = os.path.realpath(global_args.build_dir)
        rel = os.path.relpath(build_dir, build_root)
        if rel == '.':
            global_args.build_dir = os.path.join(build_root, pack)
        elif not rel.startswith('..') and not rel.startswith(pack + os.sep) and rel != pack:
            global_args.build_dir = os.path.join(build_root, pack, rel)
        # A build directory outside build/ was chosen with -B on
        # purpose; it is used as given.

        defines.append('SDKCONFIG=' + os.path.join(global_args.build_dir, 'sdkconfig'))
        base = None
        if global_args.get('preset'):
            base = _preset_cache_variable(project_dir, global_args.preset, 'SDKCONFIG_DEFAULTS')
        if not base:
            base = 'sdkconfig.defaults'
        defines.append(f'SDKCONFIG_DEFAULTS={base};sdkconfig.defaults.lang_{pack}')

    return {
        'version': '1',
        'global_options': [
            {
                'names': ['--lang-pack'],
                'help': ('Draftling language pack: ' + ', '.join(LANG_PACKS) +
                         '. Non-default packs build in build/<pack>/<board>.'),
                'envvar': ENV_VAR,
                'default': 'default',
            },
        ],
        'global_action_callbacks': [lang_pack_callback],
    }
