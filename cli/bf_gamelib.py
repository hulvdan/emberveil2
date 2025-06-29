import csv
import json as json1
import os
import re
import shutil
import tempfile
from collections import defaultdict
from dataclasses import dataclass, field
from enum import Enum
from math import radians
from pathlib import Path
from typing import Any, Callable, Iterable, TypeVar

import pyjson5 as json
import pytest
import yaml
from bf_ldtk import do_ldtk
from bf_lib import (
    ART_DIR,
    ASSETS_DIR,
    FLATBUFFERS_GENERATED_DIR,
    FLATBUFFERS_SRC_DIR,
    FLATC_PATH,
    HANDS_GENERATED_DIR,
    PIXEL_SCALE_MULTIPLIER,
    RESOURCES_DIR,
    SHADERS_DIR,
    SRC_DIR,
    TEMP_DIR,
    batched,
    hash32_file_utf8,
    log,
    recursive_mkdir,
    replace_double_spaces,
    run_command,
    timing,
    timing_mark,
)
from bf_svg import (
    OutlineType_DEFAULT,
    OutlineType_JUST_OUTLINE,
    OutlineType_JUST_SCALE,
    OutlineType_JUST_SHADOW,
    make_smooth_outlined_version_of,
)
from PIL import Image, ImageEnhance

Breakable: TypeVar = list[dict]
Line: TypeVar = list[Breakable]


def sum_tuples(a, b):
    assert len(a) == 2
    assert len(b) == 2
    return (a[0] + b[0], a[1] + b[1])


DAMAGE_TYPES = {
    "DEFAULT": 0,
    "THRUSTING": 1,
    "CRUSHING": 2,
    "FIRE": 3,
}


def texture_ids_recursive_transform(
    gamelib_recursed, transform_texture_id, transform_texture_ids_list
) -> None:
    if not isinstance(gamelib_recursed, dict):
        return

    for key, value in gamelib_recursed.items():
        if isinstance(value, dict):
            texture_ids_recursive_transform(
                value, transform_texture_id, transform_texture_ids_list
            )
        elif isinstance(key, str) and ("texture_ids" in key or "texture_id" in key):
            if "texture_ids" in key:
                assert isinstance(value, list)
                for v in value:
                    assert isinstance(v, str)
                transform_texture_ids_list(gamelib_recursed, key)
            elif "texture_id" in key:
                assert isinstance(value, str)
                transform_texture_id(gamelib_recursed, key)
        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    texture_ids_recursive_transform(
                        v, transform_texture_id, transform_texture_ids_list
                    )


def recursive_replace_transform(
    gamelib_recursed,
    key_postfix_single: str,
    key_postfix_list: str,
    codename_to_index: dict[str, int],
    *,
    root=True,
) -> list[str] | None:
    errors = None

    if not isinstance(gamelib_recursed, dict):
        return None

    for key, value in gamelib_recursed.items():
        if isinstance(value, dict):
            more_errors = recursive_replace_transform(
                value, key_postfix_single, key_postfix_list, codename_to_index, root=False
            )
            if more_errors:
                if not errors:
                    errors = more_errors
                else:
                    errors.extend(more_errors)

        elif isinstance(key, str) and (
            key.endswith((key_postfix_single, key_postfix_list))
        ):
            if key.endswith(key_postfix_list):
                assert isinstance(value, list)
                for i in range(len(value)):
                    assert isinstance(value[i], str), f"value: {value[i]}"
                    v = value[i]
                    try:
                        value[i] = codename_to_index[v]
                    except KeyError:
                        if not errors:
                            errors = []
                        errors.append(v)
            else:
                assert isinstance(value, str), f"value: {value[i]}"
                try:
                    gamelib_recursed[key] = codename_to_index[value]
                except KeyError:
                    if not errors:
                        errors = []
                    errors.append(value)

        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    more_errors = recursive_replace_transform(
                        v,
                        key_postfix_single,
                        key_postfix_list,
                        codename_to_index,
                        root=False,
                    )
                    if more_errors:
                        if not errors:
                            errors = more_errors
                        else:
                            errors.extend(more_errors)

    if root and errors:
        message = "recursive_replace_transform({}, {}):\nNot found:\n{}".format(
            key_postfix_single, key_postfix_list, "\n".join(errors)
        )
        raise AssertionError(message)

    return errors


def degrees_to_radians_recursive_transform(gamelib_recursed) -> None:
    if not isinstance(gamelib_recursed, dict):
        return

    keys_to_replace = [key for key in gamelib_recursed if key.endswith("_deg")]

    for key in keys_to_replace:
        new_key = key.replace("_deg", "_rad")
        value = gamelib_recursed.pop(key)
        assert isinstance(value, (int, float))
        gamelib_recursed[new_key] = radians(value)

    del keys_to_replace

    for value in gamelib_recursed.values():
        if isinstance(value, dict):
            degrees_to_radians_recursive_transform(value)

        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    degrees_to_radians_recursive_transform(v)


def do_creature_states_transitions(genline) -> None:
    types_list = [
        "IDLE",
        "ROLLING",
        "BLOCKED",
        "GUARD_BROKEN",
        "ATTACKING",
        "GRABBING",
        "GETTING_GRABBED",
        "KNOCKED_DOWN",
        "DYING",
        "FAKE_DYING",
    ]

    transitions_ordered_by_override_priorities = [
        # IDLE
        ("IDLE", "DYING"),
        ("IDLE", "FAKE_DYING"),
        ("IDLE", "KNOCKED_DOWN"),
        ("IDLE", "GETTING_GRABBED"),
        ("IDLE", "GUARD_BROKEN"),
        ("IDLE", "GRABBING"),
        ("IDLE", "BLOCKED"),
        ("IDLE", "ROLLING"),
        ("IDLE", "ATTACKING"),
        # ROLLING
        ("ROLLING", "DYING"),
        ("ROLLING", "FAKE_DYING"),
        ("ROLLING", "KNOCKED_DOWN"),
        ("ROLLING", "GETTING_GRABBED"),
        ("ROLLING", "IDLE"),
        # BLOCKED
        ("BLOCKED", "DYING"),
        ("BLOCKED", "FAKE_DYING"),
        ("BLOCKED", "KNOCKED_DOWN"),
        ("BLOCKED", "GETTING_GRABBED"),
        ("BLOCKED", "GUARD_BROKEN"),
        ("BLOCKED", "IDLE"),
        # GUARD_BROKEN
        ("GUARD_BROKEN", "DYING"),
        ("GUARD_BROKEN", "FAKE_DYING"),
        ("GUARD_BROKEN", "KNOCKED_DOWN"),
        ("GUARD_BROKEN", "GETTING_GRABBED"),
        ("GUARD_BROKEN", "IDLE"),
        # ATTACKING
        ("ATTACKING", "DYING"),
        ("ATTACKING", "FAKE_DYING"),
        ("ATTACKING", "KNOCKED_DOWN"),
        ("ATTACKING", "GETTING_GRABBED"),
        ("ATTACKING", "GRABBING"),
        ("ATTACKING", "IDLE"),
        # GRABBING
        ("GRABBING", "IDLE"),
        # GETTING_GRABBED
        ("GETTING_GRABBED", "DYING"),
        ("GETTING_GRABBED", "FAKE_DYING"),
        ("GETTING_GRABBED", "KNOCKED_DOWN"),
        ("GETTING_GRABBED", "IDLE"),
        # KNOCKED_DOWN
        ("KNOCKED_DOWN", "DYING"),
        ("KNOCKED_DOWN", "FAKE_DYING"),
        ("KNOCKED_DOWN", "IDLE"),
        # DYING
        # FAKE_DYING
        ("FAKE_DYING", "IDLE"),
    ]

    for a, b in transitions_ordered_by_override_priorities:
        assert a in types_list
        assert b in types_list

    transitions_grouped_by_state_type = [
        [t for t in transitions_ordered_by_override_priorities if t[0] == typee]
        for typee in types_list
    ]

    genline(
        """const int g_creatureStateTransitionOverridePriorities[CreatureState_COUNT][CreatureState_COUNT] = {{
{},
}};

""".format(
            ",\n".join(
                "  {{{}}}".format(
                    ",".join(
                        "{:2d}".format(
                            index_default_minus_1(
                                transitions_grouped_by_state_type[k],
                                (types_list[k], types_list[i]),
                            )
                            + 1
                        )
                        for i in range(len(types_list))
                    )
                )
                for k in range(len(types_list))
            ),
        )
    )


def index_default_minus_1(list_variable, value):
    try:
        return list_variable.index(value)
    except ValueError:
        return -1


def do_generate_callbacks(genline) -> None:
    with open("src/bf_game_screen.cpp", encoding="utf-8") as in_file:
        lines = in_file.readlines()

    if True:
        pattern = re.compile(r"^SCRIPTABLE_ACTION_CALLBACK\(([a-zA-Z0-9_]+)\)")
        names: list[str] = []
        for line in lines:
            match = re.match(pattern, line)
            if match:
                names.append(match.group(1))

        genline(
            """enum ScriptableActionCallbackType {{
{}
}};

#define SCRIPTABLE_ACTION_CALLBACK(name)      \\
    /* NOLINTBEGIN(misc-unused-parameters) */ \\
    void name(EntityID id, MCTX)              \\
    /* NOLINTEND(misc-unused-parameters) */
#define SCRIPTABLE_ACTION_CALLBACK_(name) ScriptableActionCallbackType_##name
using ScriptableActionCallback_t = SCRIPTABLE_ACTION_CALLBACK((*));

{}

ScriptableActionCallback_t
GetScriptableActionCallback(ScriptableActionCallbackType value) {{
    switch (value) {{
{}
    }}
    INVALID_PATH;
    return nullptr;
}}
""".format(
                "\n".join(f"  ScriptableActionCallbackType_{name}," for name in names),
                "\n".join(f"SCRIPTABLE_ACTION_CALLBACK({name});" for name in names),
                "\n".join(
                    f"  case ScriptableActionCallbackType_{name}: return {name};"
                    for name in names
                ),
            )
        )


def do_behaviors(genline) -> None:
    behaviors = (
        "IDLE",
        "RUNNING_TO_ENEMY",
        "BLOCKING",
        "MOVESET",
        "SCRIPTED",
        "SCRIPTED_INTERRUPTED",
    )

    genenum(genline, "Behavior", behaviors)

    genline(
        """const char* ToString(Behavior value) {{
  const char* values[] = {{
{}
  }};

  if (value >= ARRAY_COUNT(values)) {{
    INVALID_PATH;
    return "<-- Behavior_INVALID -->";
  }}

  return values[value];
}}
""".format(
            "\n".join(f'    "Behavior_{name}",' for name in behaviors),
        )
    )


def genenum(
    genline,
    name: str,
    values: Iterable[str],
    *,
    enum_type: str | None = None,
    add_count: bool = False,
    hex_values: bool = False,
    override_values: Iterable[Any] | None = None,
    enumerate_values: bool = False,
    add_to_string: bool = False,
    comments: list[str] | None = None,
) -> None:
    assert not (hex_values and enumerate_values)
    assert not (override_values and enumerate_values)

    if add_count or hex_values:
        assert add_count != hex_values

    string = f"enum {name}"
    if enum_type:
        string += f" : {enum_type}"
    string += " {"
    genline(string)

    def genline_with_comment(line: str, i: int) -> str:
        if comments and comments[i]:
            line += "  // " + comments[i]
        genline(line)

    if hex_values:
        for i, value in enumerate(values):
            genline_with_comment("  {}_{} = {},".format(name, value, hex(2**i)), i)
    elif override_values:
        i = 0
        for value, value2 in zip(values, override_values):
            genline_with_comment("  {}_{} = {},".format(name, value, value2), i)
            i += 1
    else:
        for i, value in enumerate(values):
            if enumerate_values:
                genline_with_comment("  {}_{} = {},".format(name, value, i), i)
            else:
                genline_with_comment("  {}_{},".format(name, value), i)

    if add_count:
        genline("  {}_COUNT,".format(name))

    genline("};\n")

    if add_to_string:
        genline(f"const char* {name}ToString({name} type) {{")
        genline("  ASSERT(type >= 0);")
        if add_count:
            genline(f"  ASSERT(type <= {len(values)});")
        else:
            genline(f"  ASSERT(type < {len(values)});")
        genline("  static constexpr const char* strings[] = {")
        for value in values:
            genline(f'    "{name}_{value}",')
        if add_count:
            genline(f'    "{name}_COUNT",')
        genline("  };")
        genline("  return strings[type];")
        genline("};\n")


CUTSCENES_TXT_PATH = ASSETS_DIR / "cutscenes.txt"
CUTSCENES_TXT_TMP_PATH = TEMP_DIR / "cutscenes.txt"


def get_default_list_from(obj: dict, key: str) -> list:
    if key not in obj:
        obj[key] = []
    return obj[key]


@timing
def convert_gamelib_json_to_binary(
    texture_name_2_id: dict[str, int], genline, genline_common, atlas_data
) -> None:
    with open(FLATBUFFERS_SRC_DIR / "gamelib.yaml", encoding="utf-8") as in_file:
        gamelib = yaml.safe_load(in_file)

    gamelib["atlas"] = atlas_data

    ldtk_data: dict
    ldtk_level_to_index: dict[str, int]
    ldtk_narrative_points: dict[str, dict[str, tuple[int, int]]]
    ldtk_gates_per_level: list[dict[str, int]]

    ldtk_data, ldtk_level_to_index, ldtk_narrative_points, ldtk_gates_per_level = do_ldtk(
        gamelib, genline
    )

    genenum(
        genline,
        "CreatureState",
        (
            "IDLE",
            "ROLLING",
            "BLOCKED",
            "GUARD_BROKEN",
            "ATTACKING",
            "GRABBING",
            "GETTING_GRABBED",
            "KNOCKED_DOWN",
            "DYING",
            "FAKE_DYING",
            "DESTROY",
        ),
        add_count=True,
        add_to_string=True,
    )

    armor_piece_types = gamelib.pop("armor_piece_types")
    genenum(genline, "ArmorPieceType", armor_piece_types, add_count=True)

    item_types = (
        "ITEM",
        "ARMOR",
        # "HELM",
        # "BODY",
    )
    genenum(genline, "ItemType", item_types)

    skill_codename_to_index = {}
    genenum(
        genline,
        "SkillType",
        (i["codename"] for i in gamelib["skills"]),
        add_count=True,
        enum_type="u16",
    )

    for i, skill in enumerate(gamelib["skills"]):
        codename = skill.pop("codename")
        skill_codename_to_index[codename] = i
        skill["name_locale"] = f"SKILL_NAME.{codename}"
        assert skill["max"] > 0
        skill["level_description_locales"] = [
            f"SKILL_LEVEL_DESCRIPTION.{codename}.{k + 1}" for k in range(skill["max"])
        ]

    # NOTE: Loading bone animations.
    bone_animation_ids = []
    bones = []
    gamelib["bones"] = bones
    if 1:
        files = (ART_DIR / "bone_animations").glob("*.json")

        for file in files:
            bone_animation_ids.append(file.stem)
            with open(file) as in_file:
                bone = json.load(in_file)

            i = 0
            for frame in bone["frames"]:
                assert len(frame["textures"]) == 1
                bone["frames"][i] = frame["textures"][0]

                texture = bone["frames"][i]
                name = texture.pop("texture_id")
                texture["shadowed_texture_id"] = f"creatures/{name}__shadowed"
                texture["outlined_texture_id"] = f"creatures/{name}__outlined"
                texture["scaled_texture_id"] = f"creatures/{name}__scaled"
                i += 1

            bones.append(bone)

    ARMOR_ANIMATION_TYPES = (
        "idle",
        "roll",
        "getting_up",
        "die",
        "animal_attack",
    )
    genenum(
        genline,
        "ArmorAnimationType",
        (i.upper() for i in ARMOR_ANIMATION_TYPES),
        add_count=True,
    )

    # NOTE: Armor types.
    armor_types = []

    found_animation_indices = set()

    for armor in gamelib["armor"]:
        codename = armor["codename"]
        armor_types.append(codename)

        armor["piece_type"] = armor_piece_types.index(armor["piece_type"])

        armor["bone_animation_ids"] = [
            index_default_minus_1(bone_animation_ids, f"{codename.lower()}_{i}")
            for i in ARMOR_ANIMATION_TYPES
        ]
        for i in armor["bone_animation_ids"]:
            if i != -1:
                assert i not in found_animation_indices
                found_animation_indices.add(i)

        if "materials" in armor:
            materials = [i["item_id"] for i in armor["materials"]]
            assert len(set(materials)) == len(
                materials
            ), "Armor '{}' must have unique materials!".format(codename)

        armor_skills = armor.get("skills", [])
        for skill in armor_skills:
            idx = skill_codename_to_index[skill["skill_id"]]
            fb_skill = gamelib["skills"][idx]
            assert skill["level"] <= fb_skill["max"]

        armor_skills.sort(key=lambda x: skill_codename_to_index[x["skill_id"]])

        get_default_list_from(armor, "skills")
        get_default_list_from(armor, "materials")

    genenum(genline, "ArmorType", armor_types, add_count=True)

    armor_type_to_index = {v: i for i, v in enumerate(armor_types)}

    added_armorset_armor = set()
    for armorset in gamelib["armorsets"]:
        codename = armorset.pop("codename")
        armorset["name_locale"] = f"ARMORSET_NAME.{codename}"

        armor_ids = armorset["armor_ids"]
        assert len(armor_ids) == 2

        for i in range(len(armor_ids)):
            armor_id = armorset["armor_ids"][i]

            assert armor_id not in added_armorset_armor
            added_armorset_armor.add(armor_id)

            armor = gamelib["armor"][armor_type_to_index[armor_id]]
            assert armor["piece_type"] == i

            assert (
                "materials" in armor
            ), "Armorset armor '{}' must have materials!".format(armor["codename"])

            gamelib["items"].append(
                {
                    "codename": armor["codename"],
                    "stack": 1,
                    "type": "ARMOR",
                    "armor_id": armor_id,
                }
            )
            armor["item_id"] = armor["codename"]

    for armor in gamelib["armor"]:
        armor.pop("codename")

    not_bound = []
    for i in range(len(bone_animation_ids)):
        if i not in found_animation_indices:
            not_bound.append(
                "'{}' was not bound to any armor!".format(bone_animation_ids[i])
            )
    if not_bound:
        raise AssertionError("\n".join(not_bound))

    degrees_to_radians_recursive_transform(gamelib)

    for key in ldtk_data:
        assert key not in gamelib
    gamelib.update(ldtk_data)

    locale_to_index: dict[str, int] = {
        key: i for i, key in enumerate(gamelib["localization"])
    }
    gamelib["localization"] = list(gamelib["localization"].values())
    localization_codename_to_translation: dict[str, str] = {
        locale: gamelib["localization"][i] for locale, i in locale_to_index.items()
    }

    consumable_types = ["ITEM", "BOMB"]

    consumable_effects = [
        "HEALING",
        "PROTECT_FIRE",
        "PROTECT_WATER",
        "PROTECT_LIGHTNING",
        "PROTECT_FROST",
        "PROTECT_DARKNESS",
        "STATUS_FIRE",
        "STATUS_WATER",
        "STATUS_LIGHTNING",
        "STATUS_FROST",
        "STATUS_DARKNESS",
    ]

    genenum(genline, "ConsumableEffect", consumable_effects)

    z_layers = gamelib.pop("render_z_layers")
    z_layer_codename_to_index = {v: i for i, v in enumerate(z_layers)}
    genenum(genline, "RenderZ", z_layers, add_count=True)

    max_takes = 0

    must_be_material_items = set()
    for value in gamelib["recipes"]:
        for item_codename in value["takes_item_ids"]:
            must_be_material_items.add(item_codename)
            max_takes = max(max_takes, len(value["takes_item_ids"]))
    for value in gamelib["harvestable_types"]:
        must_be_material_items.add(value["gives_item_id"])
    for item in gamelib["items"]:
        if item["codename"] in must_be_material_items:
            assert "consumable_type" not in item

    genline(f"constexpr int RECIPE_MAX_TAKES = {max_takes};\n")

    item_codename_to_index: dict[str, int] = {}
    for i, value in enumerate(gamelib["items"]):
        item_type = value.get("type", "ITEM")
        value["type"] = item_types.index(item_type)

        codename = value["codename"]
        item_codename_to_index[codename] = i

        value["name_locale"] = f"ITEM_NAME.{codename}"

        consumable_type = value.get("consumable_type")
        if consumable_type is not None:
            if consumable_type == "BOMB":
                assert "projectile_id" in value
            else:
                assert "projectile_id" not in value

            value["consumable_type"] = consumable_types.index(
                value.get("consumable_type")
            )

        effects = get_default_list_from(value, "effects")
        if consumable_type == "ITEM":
            assert effects
        else:
            assert not effects

        for k in range(len(effects)):
            effects[k] = consumable_effects.index(effects[k])

    creature_types = [
        "HUMANOID",
        "SEGMENTED",
        "ANIMAL",
    ]
    genenum(genline, "CreatureType", creature_types)

    mob_codename_to_index = {}
    for i, mob in enumerate(gamelib["mobs"]):
        codename = mob["codename"]
        mob["name_locale"] = "MOB_NAME.{}".format(codename)
        mob_codename_to_index[codename] = i
        mob["creature_type"] = creature_types.index(mob["creature_type"])

    if 1:  # Doing Animations
        animation_types = [
            ("IDLE", "idle"),
            ("IDLE_PHASE_2", "idle_phase2"),
            ("DYING", "dying"),
            ("DYING_PHASE_2", "dying_phase2"),
            ("GETTING_UP", "getting_up"),
            ("RESURRECT", "resurrect"),
            ("ROLL", "roll"),
            ("ANIMAL_ATTACK", "animal_attack"),
        ]

        animation_key_to_index = {}
        for i, pair in enumerate(animation_types):
            _, key = pair
            animation_key_to_index[key] = i

        genenum(genline, "AnimationType", [i[0] for i in animation_types], add_count=True)

        npcs = [i["codename"] for i in gamelib["cutscene_system"]["npcs"]]
        animations = [0] * (
            (len(mob_codename_to_index) + len(npcs)) * len(animation_types)
        )
        gamelib["animations"] = animations

        for animation in gamelib.pop("animation_sets"):
            codename = animation.pop("codename")

            extended_mob_type = mob_codename_to_index.get(codename)
            if extended_mob_type is None:
                extended_mob_type = len(mob_codename_to_index) + npcs.index(codename)

            base_index = extended_mob_type * len(animation_types)

            for key, value in animation.items():
                animations[base_index + animation_key_to_index[key]] = value
                assert value > 0

    cutscene_to_index = do_cutscenes(gamelib, ldtk_narrative_points, genline)

    if 1:
        scriptable_action_types_with_comments = (
            ("IDLE", "duration"),
            ("ASYNC_IDLE", "duration"),
            ("MOVE_TO", "pos"),
            ("LOOK_AT", "pos"),
            ("INSTANT_LOOK_AT", "pos"),
            ("ASYNC_LOOK_AT", "pos"),
            ("ATTACK", ""),
            ("ROLL", "pos (as a position towards which to roll)"),
            ("SET_RUNNING", ""),
            ("ARBITRARY_CODE", ""),
            ("STOP_PLAYER_ACTIONS", ""),
            ("RESUME_PLAYER_ACTIONS", ""),
            ("SET_BEHAVIOR_IDLE", ""),
            ("IDLE_GATE", ""),
            ("CONTROL_HIGHLIGHT", "duration"),
            ("WARLOCK_TELEPORT", "pos, pos2, actingOnNPCType"),
            ("ASYNC_WARLOCK_TELEPORT", "pos, pos2, actingOnNPCType"),
            ("FINISH_CURRENT_IDLE_GATES", ""),
            ("DIALOGUE", "dialogueElementIndex"),
            ("CLOSE_DIALOGUE_WINDOW", ""),
            ("TRANSITION", "next_level_id, next_cutscene_id"),
            ("CONSUME", "item_id"),
            ("RESTART_ACTIONS", ""),
        )
        genenum(
            genline,
            "ScriptableActionType",
            [i[0] for i in scriptable_action_types_with_comments],
            add_to_string=True,
            comments=[i[1] for i in scriptable_action_types_with_comments],
        )

    quest_codename_to_index = {}

    for i, quest in enumerate(gamelib["quests"]):
        codename = quest.pop("codename")
        assert codename not in quest_codename_to_index
        quest_codename_to_index[codename] = i

        for requires in quest.get("requires", []):
            assert (
                requires in quest_codename_to_index
            ), "Quest '{}' requires '{}'. But it wasn't previously declared!".format(
                codename, requires
            )

        quest["name_locale"] = "QUEST_NAME.{}".format(codename)
        level_gates_to_index = ldtk_gates_per_level[
            ldtk_level_to_index[quest["level_id"]]
        ]

        container = get_default_list_from(quest, "opens_gate_ids")
        for k in range(len(container)):
            container[k] = level_gates_to_index[container[k]]

        get_default_list_from(quest, "requires_quest_ids")

    drop_rates = gamelib.pop("drop_rates")
    mob_drop_mobs = []
    mob_drop_items = []
    mob_drop_pities = []
    for mob_type, drops in gamelib["mob_drops"].items():
        for drop_rate_code, item_name in drops:
            mob_drop_mobs.append(mob_codename_to_index[mob_type])
            chance, pity = drop_rates[drop_rate_code]
            mob_drop_items.append((chance, item_codename_to_index[item_name]))
            mob_drop_pities.append(pity)
    gamelib.pop("mob_drops")
    gamelib["mob_drop_mobs"] = mob_drop_mobs
    gamelib["mob_drop_items"] = mob_drop_items
    gamelib["mob_drop_pities"] = mob_drop_pities

    do_creature_states_transitions(genline)
    do_generate_callbacks(genline)
    do_behaviors(genline)

    attack_types = list(gamelib["attacks"].keys())
    attack_types.append("BACKSTAB")
    if 1:
        genenum(genline, "AttackType", attack_types, add_count=True)

        for value in gamelib["moveset_presets"].values():
            mob_list = value.get("mob", [])
            for i in range(len(mob_list)):
                mob_list[i] = attack_types.index(mob_list[i])

            player = value.get("player", {})
            for key, v in player.items():
                player[key] = attack_types.index(v)

    genenum(
        genline, "MobType", [mob["codename"] for mob in gamelib["mobs"]], add_count=True
    )

    genline("enum ConsumableType : int {")
    genline("  ConsumableType_NONE = -1,")
    for consumable_type in consumable_types:
        genline(f"  ConsumableType_{consumable_type},")
    genline("  ConsumableType_COUNT,")
    genline("};\n")

    mobtype_to_index = {}
    for i, mob in enumerate(gamelib["mobs"]):
        mobtype_to_index[mob.pop("codename")] = i

    harvestable_codename_to_index: dict[str, int] = {}
    harvestable_kinds = ["PLANTS", "MUSHROOMS", "ORE"]
    genenum(genline, "HarvestableKind", harvestable_kinds, add_count=True)
    for i, value in enumerate(gamelib["harvestable_types"]):
        codename = value.pop("codename")
        harvestable_codename_to_index[codename] = i
        value["kind"] = harvestable_kinds.index(value["kind"])
    genline("const std::vector<SkillType> g_harvestableKindToGatheringSkill = {")
    for kind in harvestable_kinds:
        genline(f"  SkillType_GATHERING_{kind},")
    genline("};\n")
    genline("const std::vector<std::tuple<f32, i8, i8>> g_harvestableKindChances = {")
    for chance, guarant, display_chance, actual_chance in (
        ("15.0f", "8", "20", "20.5"),
        ("46.5f", "4", "50", "50.9"),
    ):
        genline(f"  {{{chance}, {guarant}, {display_chance}}}, // {actual_chance}")
    genline("};\n")

    destructible_codename_to_index: dict[str, int] = {}
    destructible_flags = (
        "breaks_easily",
        "ignitable",
        "crushable",
    )
    genenum(
        genline,
        "DestructibleFlags",
        (i.upper() for i in destructible_flags),
        enum_type="u32",
        hex_values=True,
    )

    for i, value in enumerate(gamelib["destructible_types"]):
        codename = value.pop("codename")

        flags = 0
        for k, flag in enumerate(destructible_flags):
            if value.pop(flag):
                flags += 2**k
        value["flags"] = flags

        destructible_codename_to_index[codename] = i

    for attack in gamelib["attacks"].values():
        if inherits := attack.pop("_inherits", None):
            new_data = {**gamelib["attacks"][inherits]}
            assert "_inherits" not in new_data
            new_data.update(attack)
            attack.clear()
            attack.update(new_data)

    transformed_attacks = []
    for attack in gamelib["attacks"].values():
        attack["damage_type"] = DAMAGE_TYPES[attack.get("damage_type", "DEFAULT")]
        if attack.get("stamina_cost") is not None:
            attack["stamina_cost"] = gamelib["attack_stamina_costs"][
                attack["stamina_cost"]
            ]

        transformed_attacks.append(attack)

    gamelib.pop("attack_stamina_costs")
    gamelib["attacks"] = transformed_attacks

    moveset_preset_names: list[str] = []
    moveset_preset_types: set[str] = set()
    for name, moveset_preset in gamelib["moveset_presets"].items():
        moveset_preset_names.append(name)
        moveset_preset_types.add(moveset_preset.get("type", "DEFAULT"))

    moveset_preset_types: list[str] = list(moveset_preset_types)

    for moveset_preset in gamelib["moveset_presets"].values():
        moveset_preset["type"] = moveset_preset_types.index(
            moveset_preset.get("type", "DEFAULT")
        )
        if player := moveset_preset.get("player"):
            if "e" not in player:
                player["e"] = len(attack_types)
            else:
                assert (
                    "cooldown_frames" in gamelib["attacks"][player["e"]]
                ), f'Attack `{attack_types[player["e"]]}` must define `cooldown_frames` because it\'s defined as a `special` player\'s attack!'

    gamelib["moveset_presets"] = list(gamelib["moveset_presets"].values())

    for weapon in gamelib["weapons"]:
        index = -1

        weapon["damage_type"] = DAMAGE_TYPES[weapon.get("damage_type", "DEFAULT")]

        weapon_type = weapon.get("type", "DEFAULT")

        length_required = weapon_type != "SHIELD"
        if length_required:
            assert weapon.get("length") is not None
        else:
            assert "length" not in weapon

        moveset_preset_required = weapon_type != "SHIELD"
        if moveset_preset_required:
            name = weapon["moveset_preset"]
            index = moveset_preset_names.index(name)
        else:
            assert "moveset_preset" not in weapon

        weapon["moveset_preset"] = index

    weapon_types: set[str] = set()
    for weapon in gamelib["weapons"]:
        weapon_types.add(weapon.get("type", "DEFAULT"))

    weapon_types = list(weapon_types)
    weapon_types.sort()

    allowed_pools = gamelib.pop("weapon_pools")
    accumulative_counts_by_pool = [0] * len(allowed_pools)

    last_pool = 0

    for i, weapon in enumerate(gamelib["weapons"]):
        pool = weapon.pop("pool", "CUSTOM")
        pool_index = allowed_pools.index(pool)

        assert last_pool <= pool_index
        last_pool = pool_index

        for k in range(pool_index, len(accumulative_counts_by_pool)):
            accumulative_counts_by_pool[k] += 1

        if pool == "CUSTOM":
            genline(f'constexpr int Weapon_{weapon["type"]} = {i};')

        weapon["type"] = weapon_types.index(weapon.get("type", "DEFAULT"))

    genline("")

    genenum(genline, "WeaponPool", allowed_pools)
    genline("const std::vector<std::tuple<int, int>> g_weaponsByPool = {")
    for i in range(len(accumulative_counts_by_pool)):
        prev_count = 0
        count = accumulative_counts_by_pool[i]
        if i > 0:
            prev_count = accumulative_counts_by_pool[i - 1]
        genline(f"  {{{prev_count}, {count}}},")
    genline("};\n")

    genline(
        """enum WeaponType {{
{}
}};

WeaponType ToWeaponType(int value) {{
  ASSERT(value >= 0);
  ASSERT(value <= (int)WeaponType_{});
  auto result = (WeaponType)value;
  return result;
}}
""".format(
            ",\n".join(
                f"  WeaponType_{name} = {i}" for i, name in enumerate(weapon_types)
            ),
            weapon_types[-1],
        )
    )

    decoration_type_to_index = {}
    if 1:
        decoration_filepaths = list((ART_DIR / "decorations" / "tiles").glob("*.png"))

        stem_to_texture_ids = defaultdict(list)

        for filepath in decoration_filepaths:
            stem = filepath.stem
            if stem.startswith("a__"):
                stem = stem.removeprefix("a__").rsplit("_", 1)[0]

            stem_to_texture_ids[stem].append(f"tiles/{filepath.stem}")

        decoration_type_to_index = {stem: i for i, stem in enumerate(stem_to_texture_ids)}

        decoration_types = gamelib.pop("decoration_types", [])
        gamelib["decoration_types"] = []

        for stem, texture_ids in stem_to_texture_ids.items():
            value = decoration_types.get(stem.upper(), {})
            value["texture_ids"] = texture_ids
            # if "torch" in stem:
            #     value["torch_glow"] = True
            gamelib["decoration_types"].append(value)

    if 1:
        REACTIONS = (
            ("EXPLOSION", "palRed"),
            ("CHAINLIGHTNING", "palYellowOrange"),
            ("FROZEN", "palSkyblue"),
            ("VAPE", "palLightGray"),
        )
        genenum(genline, "ReactionType", (i[0] for i in REACTIONS))
        genline("const std::vector<Color> g_reactionColors = {")
        for _, color in REACTIONS:
            genline(f"  ColorBrightness({color}, 0.3f),")
        genline("};\n")

        gamelib["reaction_locales"] = [f"REACTION.{i[0]}" for i in REACTIONS]

    status_data = [
        ("FIRE", "palRed"),
        ("WATER", "palSkyblue"),
        ("LIGHTNING", "palYellowOrange"),
        ("FROST", "palSkyblue"),
        ("DARKNESS", "palPlum"),
    ]
    status_codenames = [i[0] for i in status_data]

    if 1:
        genenum(genline, "StatusType", status_codenames, add_count=True)

        genline(
            """struct StatusData {
  SkillType skillSpread      = {};
  SkillType skillDamage      = {};
  SkillType skillApplication = {};
  SkillType skillRes         = {};
  SkillType skillImmunity    = {};
};

const std::vector<StatusData> g_statusData = {"""
        )

        for codename in status_codenames:
            genline("  {")
            genline(f"    .skillSpread = SkillType_{codename}_SPREAD,")
            genline(f"    .skillDamage = SkillType_{codename}_DAMAGE,")
            genline(f"    .skillApplication = SkillType_{codename}_APPLICATION,")
            genline(f"    .skillRes = SkillType_{codename}_RES,")
            genline(f"    .skillImmunity = SkillType_{codename}_IMMUNITY,")
            genline("  },")
        genline("};\n")

        genline("const std::vector<SkillType> g_skillsRes = {")
        for codename in status_codenames:
            genline(f"  SkillType_{codename}_RES,")
        genline("};\n")

        genline("const std::vector<SkillType> g_skillsDamage = {")
        for codename in status_codenames:
            genline(f"  SkillType_{codename}_DAMAGE,")
        genline("};\n")

        genline("const std::vector<Color> g_statusColors = {")
        for _, color in status_data:
            genline(f"  {color},")
        genline("  WHITE,")
        genline("};\n")

    gamelib["status_application_locales"] = [
        f"STATUS_APPLICATION.{i}" for i in status_codenames
    ]

    COLORS = [
        ("white", "0xffffff"),
        ("black", "0x000000"),
        ("red", "0xff0000"),
        ("textColor", "0xe0d3c8"),
        ("textDisabledColor", "0xa57b73"),
        ("textErrorColor", "0xf05b5b"),
        ("backgroundColorDark", "0x452b3f"),
        ("backgroundColor", "0x56546e"),
        ("backgroundHoveredColor", "0x8a5865"),
        ("backgroundSelectedColor", "0x5479b0"),
        ("backgroundDisabledColor", "0x56546e"),
        ("backgroundHighlightedColor", "0xe08d51"),
        ("backgroundSuccessColor", "0x2c5e3b"),
        ("palLight", "0xf5eeb0"),
        ("palYellowOrange", "0xfabf61"),
        ("palOrange", "0xe08d51"),
        ("palBrown", "0x8a5865"),
        ("palDark", "0x452b3f"),
        ("palDarkGreen", "0x2c5e3b"),
        ("palGreen", "0x609c4f"),
        ("palLime", "0xc6cc54"),
        ("palSkyblue", "0x78c2d6"),
        ("palBlue", "0x5479b0"),
        ("palGray", "0x56546e"),
        ("palSlateGray", "0x839fa6"),
        ("palLightGray", "0xe0d3c8"),
        ("palRed", "0xf05b5b"),
        ("palPlum", "0x8f325f"),
        ("palPink", "0xeb6c98"),
    ]
    COLOR_NAMES = [i[0] for i in COLORS]
    if 1:
        genline_common("#define COLORS_TABLE \\")
        for i, color in enumerate(COLORS):
            line = f"  X({color[0]}, {color[1]})"
            if i != len(COLORS) - 1:
                line += " \\"
            genline_common(line)
        genline_common("")

    #         genline_common(
    #             """const std::vector<Color> g_colors {
    #   #define X(name, value) name,
    #     COLORS_TABLE
    #   #undef X
    # };
    # """
    #         )

    particle_types = [i.pop("codename") for i in gamelib["particle_types"]]
    particle_type_to_index = {p: i for i, p in enumerate(particle_types)}
    if 1:
        PARTICLE_KINDS = ["DURATIONABLE", "DISTANCEABLE"]
        genenum(genline, "ParticleKind", PARTICLE_KINDS)

        genenum(genline, "ParticleType", particle_types)

        for particle_type in gamelib["particle_types"]:
            if "kind" in particle_type:
                particle_type["kind"] = PARTICLE_KINDS.index(particle_type["kind"])
            particle_type["color"] = COLOR_NAMES.index(
                particle_type.get("color", "white")
            )

    projectile_type_to_index: dict[str, int] = {}
    if 1:
        codenames = []

        FLYING_TYPES = ["DEFAULT", "BOMB"]
        genenum(genline, "ProjectileFlyingType", FLYING_TYPES)

        for projectile_type in gamelib["projectile_types"]:
            codenames.append(projectile_type.pop("codename"))
            projectile_type["status"] = status_codenames.index(projectile_type["status"])

            ft = projectile_type["flying_type"]
            if ft == "BOMB":
                assert "min_distance" in projectile_type
            else:
                assert "min_distance" not in projectile_type
                projectile_type["min_distance"] = 0

            projectile_type["flying_type"] = FLYING_TYPES.index(ft)

        genenum(genline, "ProjectileType", codenames, add_count=True)

        projectile_type_to_index = {codename: i for i, codename in enumerate(codenames)}

    if 1:
        started = False
        with open(
            SRC_DIR / "flatbuffers" / "bf_gamelib.fbs", encoding="utf-8"
        ) as in_file:
            for line in in_file:
                if "CODEGEN_LOCALE_START" in line:
                    started = True
                    continue
                if not started:
                    continue
                if "CODEGEN_LOCALE_END" in line:
                    break

                key = line.split(":", 1)[0].strip()
                if not key:
                    continue
                gamelib[key] = key.replace("_locale", "").upper()
        assert started

    lines_with_ids: list[tuple[int, str, str]] = []

    if 1:  # cutscenes.txt line ids annotation
        cached_lines: list[tuple[CutsceneLineType, str]] = []

        def annotate_replicas_line_callback(t: CutsceneLineType, line: str) -> None:
            cached_lines.append((t, line))

        parse_cutscenes(annotate_replicas_line_callback)

        next_line_id = int(cached_lines[0][1])

        not_annotated_replicas_count = 0
        for t, line in cached_lines:
            if t == CutsceneLineType.REPLICA and "@" not in line:
                not_annotated_replicas_count += 1

        cached_lines[0] = (
            cached_lines[0][0],
            str(next_line_id + not_annotated_replicas_count) + "\n",
        )

        with open(
            CUTSCENES_TXT_TMP_PATH, "w", encoding="utf-8", newline="\n"
        ) as out_file:
            last_cutscene: str | None = None
            last_npc: str | None = None

            for t, line in cached_lines:
                if t == CutsceneLineType.CUTSCENE:
                    last_cutscene = line.strip().removeprefix("[").removesuffix("]")
                if t == CutsceneLineType.NPC:
                    last_npc = line.strip()

                if t != CutsceneLineType.REPLICA:
                    out_file.write(line)
                    continue

                assert last_cutscene is not None
                assert last_npc is not None

                id_key = "@id:"

                if id_key in line:
                    line_without_id, line_id = line.strip().split("@id:", 1)

                    lines_with_ids.append(
                        (
                            int(line_id),
                            line_without_id.strip(),
                            "{}: {}".format(last_cutscene, last_npc),
                        )
                    )

                    out_file.write(line)
                    continue

                lines_with_ids.append(
                    (
                        next_line_id,
                        line.strip(),
                        "{}: {}".format(last_cutscene, last_npc),
                    )
                )

                out_file.write(line.rstrip() + f"  {id_key}{next_line_id}\n")
                next_line_id += 1

        if hash32_file_utf8(CUTSCENES_TXT_TMP_PATH) != hash32_file_utf8(
            CUTSCENES_TXT_PATH
        ):
            CUTSCENES_TXT_TMP_PATH.replace(CUTSCENES_TXT_PATH)

    if 1:  # Localization
        csv_columns = (
            "id",
            "translated",
            "original",
            "comment",
        )

        index_to_locale = {i: codename for codename, i in locale_to_index.items()}

        languages = ["english"]

        for language in languages:
            csv_path = ASSETS_DIR / f"localization_{language}.csv"
            csv_temp_path = TEMP_DIR / f"localization_{language}.csv"

            translated_values: dict[str, str] = {}

            if csv_path.exists():
                with open(csv_path, newline="", encoding="utf-8-sig") as in_file:
                    for row in csv.DictReader(in_file):
                        translated_values[row["id"]] = row["translated"]

            with open(
                csv_temp_path,
                "w",
                newline="",
                encoding="utf-8-sig",
            ) as out_file:
                writer = csv.writer(out_file)
                writer.writerow(csv_columns)

                russian_localization: list[str] = gamelib["localizations"][0]["strings"]

                for i in range(len(locale_to_index)):
                    codename = index_to_locale[i]
                    localized = russian_localization[i]
                    writer.writerow(
                        (
                            codename,
                            translated_values.get(codename, "").strip(),
                            localized.strip(),
                            "",
                        )
                    )

                for line_id, line, comment in lines_with_ids:
                    writer.writerow(
                        (
                            str(line_id),
                            translated_values.get(str(line_id), "").strip(),
                            line.strip(),
                            comment.strip(),
                        )
                    )

            if hash32_file_utf8(csv_path) != hash32_file_utf8(csv_temp_path):
                csv_temp_path.replace(csv_path)

        russian_replicas_with_line_ids: list[tuple[str, str]] = []

        def gather_replica_line_ids(t: CutsceneLineType, line: str) -> None:
            if t == CutsceneLineType.REPLICA:
                assert "@id:" in line
                line, line_id = line.strip().split("@id:", 1)
                russian_replicas_with_line_ids.append((line, line_id))

        parse_cutscenes(gather_replica_line_ids)

        for language in languages:
            csv_path = ASSETS_DIR / f"localization_{language}.csv"

            translated_values: dict[str, str] = {}
            with open(csv_path, newline="", encoding="utf-8-sig") as in_file:
                for row in csv.DictReader(in_file):
                    translated_values[row["id"]] = row["translated"]

            strings = []

            for i in range(len(locale_to_index)):
                codename = index_to_locale[i]
                translated = translated_values[codename]
                if not translated.strip():
                    log.warn(
                        f"Localization: {language}: Translation not found '{codename}'!"
                    )

                strings.append(translated)

            replicas = []
            for _, line_id in russian_replicas_with_line_ids:
                replicas.append(
                    {
                        "blocks": [
                            {"words": i}
                            for i in process_line(
                                translated_values[line_id].strip(),
                                translated_values,
                                use_single_characters=True,
                            )
                        ]
                    }
                )

            gamelib["localizations"].append(
                {
                    "texture_id": f"ui/lang_{language}",
                    "strings": strings,
                    "replicas": replicas,
                }
            )

        russian_blocks = gamelib["localizations"][0]["replicas"]
        for line, _ in russian_replicas_with_line_ids:
            russian_blocks.append(
                {
                    "blocks": [
                        {"words": i}
                        for i in process_line(
                            line.strip(),
                            localization_codename_to_translation,
                            use_single_characters=True,
                        )
                    ]
                }
            )

        # Codegen damage numbers font codepoints.
        damage_numbers_characters = set("0123456789")
        for localization in gamelib["localizations"]:
            immunity_string = localization["strings"][locale_to_index["DMG_IMMUNITY"]]
            if immunity_string:
                for character in immunity_string:
                    damage_numbers_characters.add(character)

            for key in locale_to_index:
                patterns = ("STATUS_APPLICATION.", "REACTION.")

                for p in patterns:
                    if key.startswith(p):
                        for character in localization["strings"][locale_to_index[key]]:
                            damage_numbers_characters.add(character)

            checkpoint_string = localization["strings"][
                locale_to_index["CHECKPOINT_ACTIVATED"]
            ]
            if checkpoint_string:
                for character in checkpoint_string:
                    damage_numbers_characters.add(character)

        for char_to_remove in ("\n", "\t"):
            if char_to_remove in damage_numbers_characters:
                damage_numbers_characters.remove(char_to_remove)

        genline("const int g_damageNumbersCodepoints[] = {")
        codepoints = sorted(damage_numbers_characters)
        codepoints.append("\0")
        codepoints.append("\0")
        codepoints.append("\0")
        codepoints.append("\0")
        for batch in batched(codepoints, 8):
            string = ""
            for c in batch:
                string += f"{ord(c)}, "
            genline("  " + string.strip())
        genline("};\n")

    transform_texture_id = lambda data, key: transform_to_texture_index(
        data, key, texture_name_2_index=texture_name_2_id
    )
    transform_texture_ids_list = lambda data, key: transform_to_texture_indexes_list(
        data, key, texture_name_2_index=texture_name_2_id
    )
    texture_ids_recursive_transform(
        gamelib, transform_texture_id, transform_texture_ids_list
    )
    recursive_replace_transform(
        gamelib, "projectile_id", "projectile_ids", projectile_type_to_index
    )
    recursive_replace_transform(gamelib, "z_layer", "z_layers", z_layer_codename_to_index)
    recursive_replace_transform(gamelib, "particle", "particles", particle_type_to_index)

    with open(SHADERS_DIR / "lib.fs", encoding="utf-8") as in_file:
        shaders_lib = in_file.read()

    shaders = [
        *[("VS", i) for i in SHADERS_DIR.glob("*.vs")],
        *[("FS", i) for i in SHADERS_DIR.glob("*.fs")],
    ]

    for shader_type, shader_path in shaders:
        if shader_path.stem == "lib":
            continue

        with open(shader_path, encoding="utf-8") as in_file:
            genline(
                'auto {}_{} = R"SHADER('.format(shader_type, shader_path.stem.upper())
            )
            genline(in_file.read().replace("// INCLUDE_LIBRARY\n", shaders_lib))
            genline(')SHADER";\n')

    recursive_replace_transform(gamelib, "skill_id", "skill_ids", skill_codename_to_index)
    recursive_replace_transform(gamelib, "armor_id", "armor_ids", armor_type_to_index)
    recursive_replace_transform(gamelib, "locale", "locales", locale_to_index)
    recursive_replace_transform(gamelib, "item_id", "item_ids", item_codename_to_index)
    recursive_replace_transform(gamelib, "level_id", "level_ids", ldtk_level_to_index)
    recursive_replace_transform(gamelib, "cutscene_id", "cutscenes_id", cutscene_to_index)
    recursive_replace_transform(gamelib, "mob_id", "mob_ids", mobtype_to_index)
    recursive_replace_transform(
        gamelib, "harvestable_id", "harvestable_ids", harvestable_codename_to_index
    )
    recursive_replace_transform(
        gamelib, "destructible_id", "destructible_ids", destructible_codename_to_index
    )
    recursive_replace_transform(gamelib, "quest_id", "quest_ids", quest_codename_to_index)
    recursive_replace_transform(gamelib, "mob_type", "mob_types", mob_codename_to_index)
    recursive_replace_transform(
        gamelib, "decoration_id", "decoration_ids", decoration_type_to_index
    )

    # Создание `gamelib.bin`.
    intermediate_path = TEMP_DIR / "gamelib.intermediate.jsonc"
    intermediate_path.write_text(json1.dumps(gamelib, ensure_ascii=True, indent=4))
    run_command(
        [
            FLATC_PATH,
            "-o",
            TEMP_DIR,
            "-b",
            FLATBUFFERS_SRC_DIR / "bf_gamelib.fbs",
            intermediate_path,
        ]
    )

    intermediate_binary_path = str(intermediate_path).rsplit(".", 1)[0] + ".bin"
    shutil.move(intermediate_binary_path, RESOURCES_DIR / "gamelib.bin")


@timing
def make_atlas(path: Path) -> tuple[dict[str, int], dict]:
    assert str(path).endswith(".ftpp")

    filename_wo_extension = path.name.rsplit(".", 1)[0]

    shutil.copytree(ART_DIR / "to_do_nothing", TEMP_DIR / "art", dirs_exist_ok=True)

    cache_filepath = TEMP_DIR / ".atlas.cache"

    old_cache_value = -1
    if cache_filepath.exists():
        old_cache_value = int(cache_filepath.read_text())

    cache_value = 0
    for filepath in Path(TEMP_DIR / "art").rglob("*.png"):
        cache_value = hash(cache_value + filepath.stat().st_mtime_ns)

    if cache_value == old_cache_value:
        log.info("Skipped atlas generation - no images changed")
        timing_mark("skipped png generation")
    else:
        # Генерируем атлас из .ftpp файла. Создаются .json спецификация и .png текстура.
        log.info("Generating atlas...")
        run_command("free-tex-packer-cli --project {}".format(path))

        cache_filepath.write_text(str(cache_value))

    # Подгоняем спецификацию под наш формат.
    json_path = TEMP_DIR / (filename_wo_extension + ".json")
    with open(json_path) as json_file:
        json_data = json.load(json_file)

    found_textures: set[str] = set()

    textures: list[Any] = []
    for name_, data in json_data["frames"].items():
        name = name_.removeprefix("art/")

        assert name not in found_textures
        found_textures.add(name)

        outlined = (ART_DIR / "to_outline" / (name + ".png")).exists() or (
            "__outlined" in name
        )
        baseline = data["frame"]["h"]
        if outlined:
            baseline -= 20

        texture_data = {
            "id": -1,
            "debug_name": name,
            "size_x": data["frame"]["w"],
            "size_y": data["frame"]["h"],
            "atlas_x": data["frame"]["x"],
            "atlas_y": data["frame"]["y"],
            "scaled": (
                outlined
                or (ART_DIR / "to_scale" / (name + ".png")).exists()
                or (ART_DIR / "decorations" / "tiles" / (name + ".png")).exists()
                or (
                    ART_DIR
                    / "to_bone"
                    / (
                        name.replace("__shadowed", "")
                        .replace("__outlined", "")
                        .replace("__scaled", "")
                        + ".png"
                    )
                ).exists()
            ),
            "baseline": baseline,
        }
        textures.append(texture_data)

    texture_name_2_id = {}
    textures.sort(key=lambda x: x["debug_name"])

    for i in range(len(textures)):
        textures[i]["id"] = i
        name = textures[i]["debug_name"]
        texture_name_2_id[name] = i

    # Копируем в resources.
    shutil.copyfile(
        TEMP_DIR / (filename_wo_extension + ".png"),
        RESOURCES_DIR / (filename_wo_extension + ".png"),
    )

    return texture_name_2_id, {
        "textures": textures,
        "size_x": json_data["meta"]["size"]["w"],
        "size_y": json_data["meta"]["size"]["h"],
    }


@timing
def remove_intermediate_generation_files() -> None:
    for filepath in TEMP_DIR.rglob("*.intermediate*"):
        filepath.unlink()


def transform_to_texture_indexes_list(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
) -> None:
    textures = data[key]
    assert isinstance(textures, list)

    for i, texture_name in enumerate(textures):
        if texture_name.lower() == "undefined":
            textures[i] = -1
        else:
            assert (
                texture_name in texture_name_2_index
            ), f"Texture '{texture_name}' not found in atlas!"
            textures[i] = texture_name_2_index[texture_name]


def transform_to_texture_index(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
) -> None:
    texture_name = data[key]
    assert isinstance(texture_name, str)

    if texture_name.lower() == "undefined":
        data[key] = -1
        return

    assert (
        texture_name in texture_name_2_index
    ), f"Texture '{texture_name}' not found in atlas!"

    data[key] = texture_name_2_index[texture_name]


def listfiles_with_hashes_in_dir(path: str | Path) -> dict[str, int]:
    filepath_string_to_hash = {}

    for filepath in Path(path).rglob("*"):
        with open(filepath, "rb") as in_file:
            filepath_string_to_hash[str(filepath.relative_to(path))] = hash(
                in_file.read()
            )

    return filepath_string_to_hash


@timing
def generate_flatbuffer_files():
    hashes_for_msbuild = listfiles_with_hashes_in_dir(FLATBUFFERS_GENERATED_DIR)

    # Генерируем cpp файлы из FlatBuffer (.fbs) файлов.
    flatbuffer_files = list(SRC_DIR.rglob("*.fbs"))

    with tempfile.TemporaryDirectory() as td:

        def gen(filepaths, *args):
            command = [
                FLATC_PATH,
                *args,
                "-o",
                td,
                "--cpp",
                *list(filepaths),
            ]

            run_command(command)

        to_reflect = "bf_boner.fbs"
        gen(
            (i for i in flatbuffer_files if i.name == to_reflect),
            "--gen-object-api",
            "--reflect-names",
        )
        gen(i for i in flatbuffer_files if i.name != to_reflect)

        for file, file_hash in listfiles_with_hashes_in_dir(td).items():
            # Костыль, чтобы MSBuild не ребилдился каждый раз.
            if file_hash != hashes_for_msbuild.get(file):
                shutil.copyfile(Path(td) / file, FLATBUFFERS_GENERATED_DIR / file)


@timing
def scale_images(*filepaths) -> None:
    output_directory = TEMP_DIR / "art"

    skipped = 0

    for filepath in filepaths:
        save_to = output_directory / filepath.relative_to(filepath.parent.parent)

        s1 = filepath.stat()

        if save_to.exists():
            s2 = save_to.stat()
            if s1.st_mtime_ns == s2.st_mtime_ns:
                skipped += 1
                continue

        image = Image.open(filepath)
        if image.mode != "RGBA":
            image = image.convert("RGBA")

        image = image.resize(
            (
                PIXEL_SCALE_MULTIPLIER * image.size[0],
                PIXEL_SCALE_MULTIPLIER * image.size[1],
            ),
            Image.Resampling.NEAREST,
        )

        recursive_mkdir(save_to)
        image.save(save_to)
        os.utime(save_to, ns=(s1.st_atime_ns, s1.st_mtime_ns))

    if skipped:
        processed = len(filepaths) - skipped
        if processed:
            timing_mark("processed {}/{}".format(processed, len(filepaths)))

        timing_mark("skipped {}/{}".format(skipped, len(filepaths)))


@timing
def process_outline_images() -> None:
    cache_filepath = TEMP_DIR / ".outline.cache"

    cache = {}
    if cache_filepath.exists():
        with open(cache_filepath) as in_file:
            cache = json.load(in_file)

    files: list[tuple[Path, tuple[int, ...]]] = [
        (filepath, (OutlineType_DEFAULT,))
        for filepath in (ART_DIR / "to_outline").rglob("*.png")
    ]
    files.extend(
        (
            filepath,
            (OutlineType_JUST_SHADOW, OutlineType_JUST_OUTLINE, OutlineType_JUST_SCALE),
        )
        for filepath in (ART_DIR / "to_bone").rglob("*.png")
    )

    visited = set()

    processed = 0
    skipped = 0

    for filepath, outline_types in files:
        p = filepath.relative_to(ART_DIR)
        visited.add(str(p))

        s = filepath.stat()

        if str(p) in cache:
            if cache[str(p)] == s.st_mtime_ns:
                skipped += 1
                continue

        log.debug("Outlining '{}'...".format(p))
        make_smooth_outlined_version_of(filepath, outline_types)
        cache[str(p)] = s.st_mtime_ns
        os.utime(ART_DIR / p, ns=(s.st_atime_ns, s.st_mtime_ns))

        processed += 1

    total = processed + skipped
    if processed:
        timing_mark("processed {}/{}".format(processed, total))

    timing_mark("skipped {}/{}".format(skipped, total))

    cache = {k: v for k, v in cache.items() if k in visited}
    cache_filepath.write_text(json.dumps(cache))


@timing
def check_no_image_names_duplicated(folders: list[str]) -> None:
    existing_images = set()

    for folder_name in folders:
        base_dir = ART_DIR / folder_name

        for filepath in base_dir.rglob("*.png"):
            relative_filename = str(filepath.relative_to(base_dir))
            assert (
                relative_filename not in existing_images
            ), "Found image with the same name! {}".format(relative_filename)
            existing_images.add(relative_filename)


@timing
def check_images_in_temp_exist_in_to_process(folders: list[str]) -> None:
    temp_art_dir = TEMP_DIR / "art"
    temp_image_paths = set(
        filepath.relative_to(temp_art_dir) for filepath in temp_art_dir.rglob("*.png")
    )

    filepaths_per_folder = []
    for folder_name in folders:
        base_dir = ART_DIR / folder_name
        filepaths = set(
            filepath.relative_to(base_dir) for filepath in base_dir.rglob("*.png")
        )

        # NOTE: Файлы в `to_bone` преобразуются в outline и shadow.
        if folder_name == "to_bone":
            to_bone_filepaths = []
            for filepath in filepaths:
                for suffix in ("__shadowed.png", "__outlined.png", "__scaled.png"):
                    to_bone_filepaths.append(filepath.parent / (filepath.stem + suffix))

            filepaths_per_folder.append(to_bone_filepaths)
        else:
            filepaths_per_folder.append(filepaths)

    excessive_images = []
    for filepath in temp_image_paths:
        exists = False
        for filepaths in filepaths_per_folder:
            if filepath in filepaths:
                exists = True
                break

        if not exists:
            excessive_images.append(filepath.as_posix())

    if excessive_images:
        message = "Excessive images found!\n{}".format(
            "\n".join("{}) {}".format(i + 1, v) for i, v in enumerate(excessive_images))
        )
        raise AssertionError(message)


NOT_SET_STRING = "NOT SET"


@dataclass
class CutsceneNPCElementsParsed:
    npc: str
    elements: list[tuple[str, str]] = field(default_factory=list)


@dataclass
class CutsceneParsed:
    codename: str
    level_id: str = NOT_SET_STRING
    npcs: list[tuple[str, str, str]] = field(default_factory=list)
    npc_actions: list[CutsceneNPCElementsParsed] = field(default_factory=list)


class CutsceneLineType(Enum):
    REPLICA = 1
    CUTSCENE = 2
    NPC = 3
    OTHER = 4


def dummy_line_callback(_: CutsceneLineType, _2: str) -> None:
    pass


def parse_cutscenes(
    line_callback: Callable[[CutsceneLineType, str], None] = dummy_line_callback,
) -> list[CutsceneParsed]:
    result: list[CutsceneParsed] = []

    with open(CUTSCENES_TXT_PATH, encoding="utf-8") as in_file:
        lines = in_file.readlines()

    next_line_index = 0

    def pop_line() -> str:
        nonlocal next_line_index
        line = lines[next_line_index]
        next_line_index += 1
        return line

    current_cutscene: CutsceneParsed | None = None
    current_npc: CutsceneNPCElementsParsed | None = None
    current_cutscene_npc_codenames = []

    already_used_cutscene_codenames: list[str] = []

    line_callback(CutsceneLineType.OTHER, pop_line())
    line_callback(CutsceneLineType.OTHER, pop_line())

    while next_line_index < len(lines):
        original_line = pop_line()
        line = original_line

        # Cutscene.
        if line.startswith("["):
            line_callback(CutsceneLineType.CUTSCENE, original_line)

            cutscene_codename = line[1:].split("]", 1)[0]
            assert (
                cutscene_codename not in already_used_cutscene_codenames
            ), "Cutscene codename duplicated '{}'!".format(cutscene_codename)

            level_line = pop_line()
            line_callback(CutsceneLineType.OTHER, level_line)

            current_cutscene = CutsceneParsed(
                codename=cutscene_codename, level_id=level_line.strip()
            )
            result.append(current_cutscene)

            while True:
                cutscene_npc_line = pop_line()

                if not cutscene_npc_line.strip():
                    line_callback(CutsceneLineType.OTHER, cutscene_npc_line)
                    break

                line_callback(CutsceneLineType.NPC, cutscene_npc_line)

                npc, position_string, looks_at_string = replace_double_spaces(
                    cutscene_npc_line.strip()
                ).split(" ")

                current_cutscene.npcs.append((npc, position_string, looks_at_string))

            current_cutscene_npc_codenames.clear()
            current_cutscene_npc_codenames.extend(i[0] for i in current_cutscene.npcs)

            continue

        if not line.strip():
            line_callback(CutsceneLineType.OTHER, original_line)
            continue

        # NPC codename.
        if not line[0].isspace():
            npc = line.strip()
            assert (
                npc in current_cutscene_npc_codenames
            ), "NPC '{}' not found in cutscene '{}' acting npcs {}!".format(
                npc, current_cutscene.codename, current_cutscene_npc_codenames
            )

            if current_npc is not None:
                assert current_npc.elements

            current_npc = CutsceneNPCElementsParsed(npc=npc)
            current_cutscene.npc_actions.append(current_npc)

            line_callback(CutsceneLineType.NPC, original_line)

            continue

        # NPC elements.
        else:
            line = replace_double_spaces(line.strip())

            # NPC game action.
            if line[0] == "@":
                action_codename, arguments = line.split(" ", 1)
                current_npc.elements.append((action_codename[1:], arguments))

                line_callback(CutsceneLineType.OTHER, original_line)

            # NPC replica.
            else:
                current_npc.elements.append(
                    ("REPLICA", line.rsplit("@id:", 1)[0].strip())
                )

                line_callback(CutsceneLineType.REPLICA, original_line)

            continue

    return result


def do_cutscenes(gamelib, ldtk_narrative_points, genline) -> dict[str, int]:
    cutscene_system = gamelib["cutscene_system"]

    all_game_npcs = [i.pop("codename") for i in cutscene_system["npcs"]]
    assert len(set(all_game_npcs)) == len(all_game_npcs)
    genenum(genline, "NPCType", all_game_npcs, add_count=True)

    parsed_cutscenes = parse_cutscenes()

    cutscenes = [i.codename for i in parsed_cutscenes]
    genenum(genline, "CutsceneType", cutscenes, add_count=True, enumerate_values=True)

    directions = {
        "RIGHT_BOTTOM": (10, 1),
        "RIGHT_TOP": (10, -1),
        "BOTTOM_RIGHT": (1, 10),
        "TOP_RIGHT": (1, -10),
        "LEFT_BOTTOM": (-10, 1),
        "LEFT_TOP": (-10, -1),
        "BOTTOM_LEFT": (-1, 10),
        "TOP_LEFT": (-1, -10),
        "RIGHT": (1, 0),
        "LEFT": (-1, 0),
    }

    ELEMENT_TYPE_TO_INDEX = {
        "REPLICA": 0,
        "MOVE_TO": 1,
        "ASYNC_MOVE_TO": 2,
        "ASYNC_LOOK_AT": 3,
        "CONSUME": 4,
        "ASYNC_WARLOCK_TELEPORT": 5,
        "WARLOCK_TELEPORT": 6,
        "TRANSITION": 7,
        "IDLE": 8,
        "ASYNC_IDLE": 9,
    }

    def parse_pos(level_id: str, string: str) -> tuple[int, int]:
        if string.startswith("["):
            return tuple(json.loads(string))

        return ldtk_narrative_points[level_id][string]

    cutscene_npc_codenames: list[str] = []
    npc_positions: dict[str, tuple[int, int]] = {}

    def parse_looks_at(
        level_id: str, string, looks_from: tuple[int, int]
    ) -> tuple[int, int]:
        if string.startswith("["):
            p = sum_tuples(looks_from, tuple(json.loads(string)))
        elif string in directions:
            p = sum_tuples(looks_from, directions[string])
        elif string in cutscene_npc_codenames:
            p = npc_positions[string]
        else:
            p = ldtk_narrative_points[level_id][string]

        return sum_tuples((0.5, 0.5), p)

    transformed_cutscenes = []
    gamelib["cutscene_system"]["cutscenes"] = transformed_cutscenes

    gamelib["localizations"] = [
        {
            "texture_id": "ui/lang_russian",
            "strings": gamelib.pop("localization"),
            "replicas": [],
        }
    ]

    next_replica_id = 0

    for cutscene in parsed_cutscenes:
        transformed_cutscene = {
            "codename": cutscene.codename,
            "level_id": cutscene.level_id,
            "camera_pos": ldtk_narrative_points[cutscene.level_id]["camera"],
            "elements": [],
        }

        transformed_cutscenes.append(transformed_cutscene)

        npcs_to_check = [i[0] for i in cutscene.npcs]
        npcs_to_check.extend(i.npc for i in cutscene.npc_actions)

        for npc in npcs_to_check:
            assert npc in all_game_npcs, "Not found NPC '{}' in all_game_npcs: {}".format(
                npc, all_game_npcs
            )

        for npc_actions in cutscene.npc_actions:
            for element in npc_actions.elements:
                assert (
                    element[0] in ELEMENT_TYPE_TO_INDEX
                ), "Not found element '{}' in {}".format(
                    element[0], list(ELEMENT_TYPE_TO_INDEX.keys())
                )

        npc_positions.clear()
        cutscene_npc_codenames.clear()
        for npc_data in cutscene.npcs:
            npc_positions[npc_data[0]] = parse_pos(cutscene.level_id, npc_data[1])
            cutscene_npc_codenames.append(npc_data[0])

        transformed_cutscene["npcs"] = [
            {
                "npc": all_game_npcs.index(i[0]),
                "pos": parse_pos(cutscene.level_id, i[1]),
                "looks_at": parse_looks_at(cutscene.level_id, i[2], npc_positions[i[0]]),
            }
            for i in cutscene.npcs
        ]

        for npc_actions in cutscene.npc_actions:
            for element, arguments_ in npc_actions.elements:
                data = {
                    "npc": all_game_npcs.index(npc_actions.npc),
                    "type": ELEMENT_TYPE_TO_INDEX[element],
                }
                transformed_cutscene["elements"].append(data)

                if element == "REPLICA":
                    data["replica_id"] = next_replica_id
                    next_replica_id += 1
                    continue

                arguments = arguments_.split(" ")

                if element in ("MOVE_TO", "ASYNC_MOVE_TO"):
                    assert len(arguments) == 1
                    data["pos"] = parse_pos(cutscene.level_id, arguments[0])
                    npc_positions[npc_actions.npc] = data["pos"]

                elif element == "ASYNC_LOOK_AT":
                    assert len(arguments) == 1
                    data["pos"] = parse_looks_at(
                        cutscene.level_id, arguments[0], npc_positions[npc_actions.npc]
                    )

                elif element == "CONSUME":
                    assert len(arguments) == 1
                    data["item_id"] = arguments[0]

                elif element in ("ASYNC_WARLOCK_TELEPORT", "WARLOCK_TELEPORT"):
                    assert len(arguments) == 3
                    assert arguments[0] in cutscene_npc_codenames
                    data["acting_on_npc_type"] = all_game_npcs.index(arguments[0])
                    data["pos"] = parse_pos(cutscene.level_id, arguments[1])

                    data["pos2"] = parse_looks_at(
                        cutscene.level_id, arguments[0], data["pos"]
                    )
                    npc_positions[npc_actions.npc] = data["pos"]

                elif element == "TRANSITION":
                    assert len(arguments) == 2
                    assert arguments[0] in ("CUTSCENE", "GAMEPLAY")

                    is_cutscene = arguments[0] == "CUTSCENE"
                    if is_cutscene:
                        data["next_cutscene_id"] = arguments[1]
                    else:
                        data["next_level_id"] = arguments[1]

                elif element == "IDLE":
                    assert len(arguments) == 1
                    data["frames"] = int(arguments[0])

                elif element == "ASYNC_IDLE":
                    assert len(arguments) == 1
                    data["frames"] = int(arguments[0])

                else:
                    assert False

    return {codename: i for i, codename in enumerate(cutscenes)}


@timing
def do_generate() -> None:
    with open(
        HANDS_GENERATED_DIR / "bf_codegen.cpp", "w", encoding="utf-8"
    ) as codegen_file, open(
        HANDS_GENERATED_DIR / "bf_codegen_common.cpp", "w", encoding="utf-8"
    ) as codegen_common_file:
        for file in (codegen_file, codegen_common_file):
            file.write(
                """// automatically generated by cli.py, do not modify
#pragma once

"""
            )

        def genline(value):
            codegen_file.write(value)
            codegen_file.write("\n")

        def genline_common(value):
            codegen_common_file.write(value)
            codegen_common_file.write("\n")

        generate_flatbuffer_files()

        remove_intermediate_generation_files()

        scale_images(
            *(ART_DIR / "to_scale").rglob("*.png"),
            *(ART_DIR / "decorations").rglob("*.png"),
        )
        process_outline_images()

        for filepath in ART_DIR.glob("darken__*.png"):
            image = Image.open(filepath)
            if image.mode != "RGBA":
                image = image.convert("RGBA")
            image = ImageEnhance.Brightness(image).enhance(0.66667)
            image.save(RESOURCES_DIR / filepath.name.removeprefix("darken__"))

        check_no_image_names_duplicated(
            (
                "to_do_nothing",
                "to_outline",
                "to_scale",
                "decorations",
            )
        )
        check_images_in_temp_exist_in_to_process(
            (
                "to_do_nothing",
                "to_outline",
                "to_scale",
                "to_bone",
                "decorations",
            )
        )

        texture_name_2_id, atlas_data = make_atlas(
            Path("src") / "assets" / "art" / "atlas.ftpp"
        )

        convert_gamelib_json_to_binary(
            texture_name_2_id, genline, genline_common, atlas_data
        )


SPECIAL_TOKENS = ("#",)


def consume_token(block: str, index: int) -> tuple[str, int]:
    assert " " not in block

    first_is_spec = False
    found_spec = False

    for i in range(index, len(block)):
        if block[i] in SPECIAL_TOKENS:
            found_spec = True
            if i == index:
                first_is_spec = True
            break

    if found_spec and not first_is_spec:
        i -= 1

    return block[index : i + 1], i + 1


@pytest.mark.parametrize(
    ("value", "result"),
    [
        (("aboba", 0), ("aboba", 5)),
        (("aboba.", 0), ("aboba.", 6)),
        (("#aboba#", 0), ("#", 1)),
        (("#aboba#", 1), ("aboba", 6)),
        (("#aboba#", 6), ("#", 7)),
        (("#aboba#.", 0), ("#", 1)),
        (("#aboba#.", 1), ("aboba", 6)),
        (("#aboba#.", 6), ("#", 7)),
        (("#aboba#.", 7), (".", 8)),
    ],
)
def test_consume_token(value, result):
    result_ = consume_token(*list(value))
    assert result_ == result


STYLE_ANGRY = 1
MAX_CHARS_PER_LINE = 128


def process_line(
    line: str,
    localization_codename_to_translation,
    *,
    use_single_characters: bool = False,
):
    assert "\n" not in line

    line = replace_double_spaces(line)
    line = line.replace("- ", "— ")

    blocks = []

    current_style = 0

    line = line.format(**localization_codename_to_translation)
    assert len(line) <= MAX_CHARS_PER_LINE

    blocks_to_process = line.split(" ")

    for block_to_process in blocks_to_process:
        block = []
        next_token_index = 0

        while next_token_index < len(block_to_process):
            token, next_token_index = consume_token(block_to_process, next_token_index)

            if token == "#":
                assert current_style in (0, STYLE_ANGRY)

                if current_style == 0:
                    current_style = STYLE_ANGRY
                elif current_style == STYLE_ANGRY:
                    current_style = 0

            else:
                if use_single_characters:
                    for character in token:
                        block.append({"style": current_style, "text": character})
                else:
                    block.append({"style": current_style, "text": token})

        blocks.append(block)

    return blocks


@pytest.mark.parametrize(
    ("value", "result"),
    [
        (
            "aboba",
            [[{"style": 0, "text": "aboba"}]],
        ),
        (
            "aboba.",
            [[{"style": 0, "text": "aboba."}]],
        ),
        (
            "#aboba#.",
            [
                [
                    {"style": 1, "text": "aboba"},
                    {"style": 0, "text": "."},
                ],
            ],
        ),
        (
            "this is good",
            [
                [{"style": 0, "text": "this"}],
                [{"style": 0, "text": "is"}],
                [{"style": 0, "text": "good"}],
            ],
        ),
        (
            "this. is good.",
            [
                [{"style": 0, "text": "this."}],
                [{"style": 0, "text": "is"}],
                [{"style": 0, "text": "good."}],
            ],
        ),
        (
            "this?? is good!!!?",
            [
                [{"style": 0, "text": "this??"}],
                [{"style": 0, "text": "is"}],
                [{"style": 0, "text": "good!!!?"}],
            ],
        ),
        (
            "this?? is #good#!!!?",
            [
                [{"style": 0, "text": "this??"}],
                [{"style": 0, "text": "is"}],
                [
                    {"style": 1, "text": "good"},
                    {"style": 0, "text": "!!!?"},
                ],
            ],
        ),
        (
            "{COMRADE1}",
            [
                [{"style": 0, "text": "Комрад1"}],
            ],
        ),
    ],
)
def test_process_line(value, result):
    result_ = process_line(value, {"COMRADE1": "Комрад1"})
    assert result_ == result
