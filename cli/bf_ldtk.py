# ruff: noqa: ERA001, N815

import json
from math import atan2
from pathlib import Path
from typing import Any, Iterable

import lxml.etree as le
from bf_lib import (
    ART_DIR,
    ASSETS_DIR,
    RESOURCES_DIR,
    SRC_DIR,
    eprint,
    run_command,
    timing,
)
from pydantic import BaseModel
from pydantic_core import from_json as pydantic_from_json

DIRECTIONS = ["right", "down", "left", "up"]


def do_strip_tiled_and_codegen_layer_ids(level_identifiers: Iterable[str]) -> None:
    for i, level_identifier in enumerate(level_identifiers):
        filename = "{:04d}_{}.tmx".format(i + 1, level_identifier)

        with open(SRC_DIR / "assets" / "level" / "tiled" / filename) as f:
            doc = le.parse(f)
            for elem in doc.xpath("/map/objectgroup"):
                parent = elem.getparent()
                parent.remove(elem)

            for elem in doc.xpath("/map/tileset"):
                name = elem.attrib["name"]
                if name in ("decorations", "forest", "forest_resources"):
                    elem.getparent().remove(elem)

            for elem in doc.xpath("/map/tileset/image"):
                source = elem.attrib["source"]

                if source.endswith(".intgrid.png"):
                    tileset_elem = elem.getparent()
                    tileset_elem.getparent().remove(tileset_elem)
                    continue

                elem.attrib["source"] = Path(elem.attrib["source"]).name

            for elem in doc.xpath("/map/layer"):
                if elem.attrib["name"].endswith("_values"):
                    elem.getparent().remove(elem)

            data = le.tostring(doc).decode("utf-8")
            (RESOURCES_DIR / f"{level_identifier}.tmx").write_text(data, encoding="utf-8")


def ldtk_transform_field_names(data) -> None:
    """Перевод полей формата `__aboba` в `aboba_`."""

    if not isinstance(data, dict):
        return

    for key in [k for k in data if k.startswith("__")]:
        new_key = key[2:] + "_"
        data[new_key] = data.pop(key)

    for value in data.values():
        if isinstance(value, dict):
            ldtk_transform_field_names(value)
        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    ldtk_transform_field_names(v)


class Entity(BaseModel):
    identifier: str
    # uid: int
    # tags: [str]
    # exportToToc: bool
    # allowOutOfBounds: bool
    # doc: null
    # width: 16
    # height: 16
    # resizableX: false
    # resizableY: false
    # minWidth: null
    # maxWidth: null
    # minHeight: null
    # maxHeight: null
    # keepAspectRatio: true
    # tileOpacity: 1
    # fillOpacity: 1
    # lineOpacity: 1
    # hollow: false
    # color: "#63C74D"
    # renderMode: "Ellipse"
    # showName: true
    # tilesetId: null
    # tileRenderMode: "FitInside"
    # tileRect: null
    # uiTileRect: null
    # nineSliceBorders: []
    # maxCount: 0
    # limitScope: "PerLevel"
    # limitBehavior: "MoveLastOne"
    # pivotX: 0.5
    # pivotY: 0.5
    # fieldDefs: []


class TilesetTileCustomdata(BaseModel):
    data: str
    tileId: int


class TilesetTileEnumTag(BaseModel):
    enumValueId: str
    tileIds: list[int]


class Tileset(BaseModel):
    identifier: str
    cHei_: int
    cWid_: int
    customData: list[TilesetTileCustomdata]
    enumTags: list[TilesetTileEnumTag]
    uid: int


class IntGridValueDef(BaseModel):
    value: int
    identifier: str


class LayerDef(BaseModel):
    identifier: str
    intGridValues: list[IntGridValueDef]


class Defs(BaseModel):
    entities: list[Entity]
    tilesets: list[Tileset]
    layers: list[LayerDef]


class FieldInstance(BaseModel):
    identifier_: str  # direction
    type_: str
    # __type  :  LocalEnum.direction
    value_: Any  # right
    # __tile  :  null
    # defUid  :  60
    # realEditorValues: list


def field_function(self, identifier: str, default: Any = None) -> Any:
    try:
        value = next(
            field for field in self.fieldInstances if field.identifier_ == identifier
        )
        return value.value_

    except StopIteration:
        if default is not None:
            return default

        eprint(
            'Field "{}" not found !\nAvailable values are: {}'.format(
                identifier,
                ", ".join(field.identifier_ for field in self.fieldInstances),
            )
        )
        raise


def field_ref_function(self, identifier: str, reference_layer) -> Any:
    try:
        value = next(
            field for field in self.fieldInstances if field.identifier_ == identifier
        )

        assert value.type_ == "EntityRef"
        assert reference_layer is not None
        if value.value_ is not None:
            return get_referenced_entity(reference_layer, value.value_["entityIid"])
        return None

    except StopIteration:
        eprint(
            'Field "{}" not found !\nAvailable values are: {}'.format(
                identifier,
                ", ".join(field.identifier_ for field in self.fieldInstances),
            )
        )
        raise


class EntityInstance(BaseModel):
    identifier_: str  # "entities"
    grid_: tuple[int, int]
    pivot_: tuple[float, float]
    # tile_: TileEntityField | None
    tags_: list[str]
    # tile_: null
    # smartColor_: "#63C74D"
    iid: str
    width: int  # 16
    height: int  # 16
    # defUid: 51
    px: tuple[int, int]
    fieldInstances: list[FieldInstance]
    # worldX_: int  # 104
    # worldY_: int  # -61

    @property
    def size(self) -> list[int]:
        assert self.width % 16 == 0
        assert self.height % 16 == 0
        x = self.width // 16
        y = self.height // 16
        return [x, y]

    field = field_function
    field_ref = field_ref_function

    def direction_field(self, identifier: str) -> float | None:
        field = self.field(identifier)

        if field is None:
            return 0

        x = field["cx"]
        y = field["cy"]

        return atan2(y - self.grid_[1], x - self.grid_[0])


class LayerTile(BaseModel):
    px: tuple[int, int]
    # src: list[int]


class LayerInstance(BaseModel):
    identifier_: str  # "entities"
    # type_: str  # "Entities"
    cWid_: int  # 71
    cHei_: int  # 54
    # gridSize_: int  # 16
    # opacity_: int # 1
    # pxTotalOffsetX_: int # 0
    # pxTotalOffsetY_: int # 0
    # tilesetDefUid_: null
    # tilesetRelPath_: null
    # iid: "94e96550-c210-11ef-acf4-8363d5ef1ebd"
    # levelId: 0
    # layerDefUid: 52
    # pxOffsetX: 0
    # pxOffsetY: 0
    # visible: true
    # optionalRules: []
    intGridCsv: list[int]
    # autoLayerTiles: []
    # seed: 5679830
    # overrideTilesetUid: null
    gridTiles: list[LayerTile]
    entityInstances: list[EntityInstance]

    @property
    def size(self) -> list[int]:
        return [self.cWid_, self.cHei_]


class Level(BaseModel):
    identifier: str
    iid: str
    uid: int
    worldX: int
    worldY: int
    worldDepth: int
    # pxWid: int
    # pxHei: int
    # bgColor_: "#000000"
    # bgColor: null
    # useAutoIdentifier: true
    # bgRelPath: null
    # bgPos: null
    # bgPivotX: 0.5
    # bgPivotY: 0.5
    # smartColor_: "#737373"
    # bgPos_: null
    # externalRelPath: null
    fieldInstances: list[FieldInstance]
    layerInstances: list[LayerInstance]
    # neighbours_: [

    field = field_function
    field_ref = field_ref_function


class LDtk(BaseModel):
    defs: Defs
    levels: list[Level]


def try_get_single_entity(layer: LayerInstance, identifier: str) -> EntityInstance:
    found = None
    for e in layer.entityInstances:
        if e.identifier_ == identifier:
            assert found is None
            found = e

    return found


def get_single_entity(layer: LayerInstance, identifier: str) -> EntityInstance:
    found = try_get_single_entity(layer, identifier)
    assert found is not None
    return found


def get_referenced_entity(layer: LayerInstance, referenced_iid: str) -> EntityInstance:
    return next(e for e in layer.entityInstances if e.iid == referenced_iid)


BOSSES = {
    "demon": 0,
    "worm": 1,
    "summoner": 2,
}


@timing
def do_ldtk(gamelib, genline):  # noqa: ARG001
    with open(SRC_DIR / "assets" / "level.ldtk") as in_file:
        data = in_file.read()

    loaded_json = pydantic_from_json(data)
    ldtk_transform_field_names(loaded_json)
    d = LDtk.model_validate(loaded_json)

    do_strip_tiled_and_codegen_layer_ids(i["identifier"] for i in loaded_json["levels"])

    root_data = {}
    levels_data = []
    narrative_points: dict[str, dict[str, tuple[int, int]]] = {}

    gates_per_level_to_index = []

    floor_layer_def = None
    for layer in d.defs.layers:
        if layer.identifier == "floor":
            floor_layer_def = layer
            break
    assert floor_layer_def

    floor_type_codenames = [i.pop("codename") for i in gamelib["floor_types"]]
    floor_sound_type_codenames = [i["codename"] for i in gamelib.pop("floor_sound_types")]
    FLOOR_TYPES = {
        i.value: floor_type_codenames.index(i.identifier.upper())
        for i in floor_layer_def.intGridValues
    }
    for floor_type in gamelib["floor_types"]:
        floor_type["floor_sound_type"] = floor_sound_type_codenames.index(
            floor_type["floor_sound_type"]
        )

    for level in d.levels:
        entities_layer: LayerInstance | None = None
        walls_layer: LayerInstance | None = None
        liquid_layer: LayerInstance | None = None
        floor_layer: LayerInstance | None = None
        placeholders_layer: LayerInstance | None = None
        decorations_layer: LayerInstance | None = None
        for layer in level.layerInstances:
            if layer.identifier_ == "entities":
                entities_layer = layer
            elif layer.identifier_ == "walls":
                walls_layer = layer
            elif layer.identifier_ == "placeholders":
                placeholders_layer = layer
            elif layer.identifier_ == "liquid":
                liquid_layer = layer
            elif layer.identifier_ == "floor":
                floor_layer = layer
            elif layer.identifier_ == "decorations":
                decorations_layer = layer

        assert entities_layer
        assert walls_layer
        assert placeholders_layer
        assert liquid_layer
        assert decorations_layer

        points = [
            e for e in entities_layer.entityInstances if e.identifier_ == "narrative"
        ]

        if points:
            points.sort(key=lambda x: x.field("name"))
            narrative_points[level.identifier] = {
                point.field("name"): point.grid_ for point in points
            }

        bosses = []

        for trigger in (
            e for e in entities_layer.entityInstances if e.identifier_ == "trigger"
        ):
            boss = trigger.field_ref("boss", entities_layer)
            door = trigger.field_ref("door", entities_layer)
            gate = trigger.field_ref("gate", entities_layer)
            arena = trigger.field_ref("arena", entities_layer)

            v = {
                "boss_type": BOSSES[boss.field("boss")],
                "boss_pos": boss.grid_,
                "trigger_area": {
                    "pos": trigger.grid_,
                    "size": trigger.size,
                },
                "arena_area": {
                    "pos": arena.grid_,
                    "size": arena.size,
                },
            }

            if door:
                v["door_set"] = True
                v["door"] = door.grid_

            if gate:
                v["gate_set"] = True
                v["gate"] = {
                    "pos": gate.grid_,
                    "size": gate.size,
                }

            bosses.append(v)

        fire_traps = [
            {
                "pos": e.grid_,
                "direction": DIRECTIONS.index(e.field("direction")),
                "projectile_type": 0,
                "always_active": True,
                "cooldown": e.field("cooldown"),
                "knocks_down": e.field("knocks_down"),
            }
            for e in entities_layer.entityInstances
            if e.identifier_ == "fire"
        ]
        lightning_traps = [
            {
                "pos": e.grid_,
                "direction": DIRECTIONS.index(e.field("direction")),
                "projectile_type": 1,
                "always_active": False,
                "cooldown": 0,
                "knocks_down": False,
            }
            for e in entities_layer.entityInstances
            if e.identifier_ == "lightning_bolt"
        ]

        traps = []
        traps.extend(fire_traps)
        traps.extend(lightning_traps)

        lightning_bolt_ids = [
            e.iid
            for e in entities_layer.entityInstances
            if e.identifier_ == "lightning_bolt"
        ]

        wind_traps = []

        for e in (i for i in entities_layer.entityInstances if i.identifier_ == "wind"):
            wind_traps.append(
                {
                    "direction": DIRECTIONS.index(e.field("direction")),
                    "pos": e.grid_,
                    "size": e.size,
                }
            )

        buttons = []
        for button in (
            e for e in entities_layer.entityInstances if e.identifier_ == "button"
        ):
            b_type = 0
            visualization = 0
            trap = -1

            ref = button.field_ref("triggers", entities_layer)

            if ref.identifier_ == "spawner":
                b_type = 1
            elif ref.identifier_ == "visualization":
                b_type = 2
                visualization = ref.field("type")
            elif ref.identifier_ == "lightning_bolt":
                b_type = 3
                trap = len(fire_traps) + lightning_bolt_ids.index(ref.iid)
            assert b_type

            data = {
                "pos": button.grid_,
                "visualization": visualization,
                "trap": trap,
                "effect_pos": ref.grid_,
                "type": b_type,
            }

            buttons.append(data)

        teleport_positions_ = [
            e
            for e in entities_layer.entityInstances
            if e.identifier_ == "teleport_position"
        ]

        primary_is_only_one = False
        for teleport_pos in teleport_positions_:
            if teleport_pos.field("primary"):
                assert not primary_is_only_one
                primary_is_only_one = True
        assert primary_is_only_one

        teleport_positions = [next(e for e in teleport_positions_ if e.field("primary"))]

        while True:
            next_teleport = teleport_positions[-1].field_ref("next", entities_layer)
            if next_teleport.iid == teleport_positions[0].iid:
                break

            teleport_positions.append(next_teleport)

        pickupables = [
            {
                "pos": e.grid_,
                "item_ids": [f.upper() for f in e.field("item_ids")],
                "counts": e.field("counts"),
            }
            for e in entities_layer.entityInstances
            if e.identifier_ == "pickupable"
        ]

        for pickupable in pickupables:
            assert len(pickupable["item_ids"]) == len(pickupable["counts"])
            assert len(pickupable["item_ids"]) > 0

        max_size = (
            max(walls_layer.size[0], placeholders_layer.size[0]),
            max(walls_layer.size[1], placeholders_layer.size[1]),
        )

        colliders_map = bytearray(0 for _ in range(max_size[0] * max_size[1]))

        for e in (
            i for i in entities_layer.entityInstances if i.identifier_ == "collider"
        ):
            for y in range(e.height // 16):
                for x in range(e.width // 16):
                    y_ = e.grid_[1] + y
                    x_ = e.grid_[0] + x
                    colliders_map[y_ * max_size[0] + x_] = 1

        walls = []
        liquid = []
        floor_step_types = []
        placeholders_tileset = next(
            t for t in d.defs.tilesets if t.identifier == "placeholders"
        )

        placeholder_tile_collisions: dict[int, int] = {}
        for tag in placeholders_tileset.enumTags:
            for tile_id in tag.tileIds:
                if tag.enumValueId == "full":
                    placeholder_tile_collisions[tile_id] = 1

        for y in range(max_size[1]):
            for x in range(max_size[0]):
                wall = 0
                liq = 0

                if (x < walls_layer.size[0]) and (y < walls_layer.size[1]):
                    if not colliders_map[y * max_size[0] + x]:
                        wall = walls_layer.intGridCsv[walls_layer.size[0] * y + x]
                if (x < liquid_layer.size[0]) and (y < liquid_layer.size[1]):
                    liq = liquid_layer.intGridCsv[liquid_layer.size[0] * y + x]

                walls.append(wall)
                liquid.append(liq)

                s = 0
                if liq:
                    s = floor_type_codenames.index("WATER")
                elif (x < floor_layer.size[0]) and (y < floor_layer.size[1]):
                    t = y * floor_layer.size[0] + x
                    s = FLOOR_TYPES.get(floor_layer.intGridCsv[t], 0)

                floor_step_types.append(s)

        for tile in placeholders_layer.gridTiles:
            t = tile.px[1] // 16 * max_size[0] + tile.px[0] // 16
            v = walls[t]
            if v == 0:
                walls[t] = 1

        visualizations_subdata = {}

        if True:
            e = try_get_single_entity(entities_layer, "visualization_movement")
            visualizations_subdata["visualization_movement_pos"] = (
                (0, 0) if e is None else e.grid_
            )

            def vis(type_value: int) -> tuple[int, int]:
                try:
                    return next(
                        e.grid_
                        for e in entities_layer.entityInstances  # noqa: B023
                        if e.identifier_ == "visualization"
                        and e.field("type") == type_value
                    )
                except StopIteration:
                    return (0, 0)

            visualizations_subdata["visualization_attack_pos"] = vis(1)
            visualizations_subdata["visualization_backstab_pos"] = vis(2)
            visualizations_subdata["visualization_roll_pos"] = vis(3)
            visualizations_subdata["visualization_run_pos"] = vis(4)

        decorations = [
            {
                "decoration_id": e.identifier_.removeprefix("d_"),
                "pos": e.px,
            }
            for e in decorations_layer.entityInstances
            if e.identifier_ != "d_collider"
        ]
        decorations.sort(key=lambda x: x["pos"][1])

        decoration_colliders = [
            {
                "pos": e.px,
                "size": (e.width, e.height),
            }
            for e in decorations_layer.entityInstances
            if e.identifier_ == "d_collider"
        ]

        gates = {}
        gates_ = []
        gates_per_level_to_index.append(gates)

        pickaxers = [
            e for e in entities_layer.entityInstances if e.identifier_ == "pickaxer"
        ]

        for i, gate in enumerate(
            e for e in entities_layer.entityInstances if e.identifier_ == "gate"
        ):
            g = gate.field("gate")
            assert g

            gate_data = {
                "codename": "{}|{}".format(level.identifier, gate.field("gate")),
                "pos": gate.grid_,
                "size": gate.size,
            }

            for pickaxer in pickaxers:
                if pickaxer.field_ref("gate", entities_layer).iid == gate.iid:
                    gate_data["pickaxer_set"] = True
                    gate_data["pickaxer_pos"] = pickaxer.grid_
                    break

            gates_.append(gate_data)
            gates[g] = i

        levels_data.append(
            {
                "gates": gates_,
                "codename": level.identifier,
                "tmx": "resources/{}.tmx".format(level.identifier),
                "player": teleport_positions[0].grid_,
                "size": max_size,
                "walls": walls,
                "colliders": [
                    {
                        "pos": e.grid_,
                        "size": (e.width // 16, e.height // 16),
                    }
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "collider"
                ],
                "liquid": liquid,
                "floor_step_types": floor_step_types,
                "teleport_positions": [e.grid_ for e in teleport_positions],
                "mobs": [
                    {
                        "pos": e.grid_,
                        "mob_type": e.field("mob").upper(),
                        "orc_type": {"default": 0, "spearman": 1, "wizard": 2}[
                            e.field("orc_type")
                        ],
                        "direction_set": e.field("looks_at") is not None,
                        "direction": e.direction_field("looks_at"),
                    }
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "mob"
                ],
                "traps": traps,
                "wind_traps": wind_traps,
                "checkpoints": [
                    e.grid_
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "checkpoint"
                ],
                "bosses": bosses,
                "destructibles": [
                    {
                        "pos": e.grid_,
                        "destructible_id": e.identifier_.upper(),
                        "respawns": e.field("respawns", False),
                    }
                    for e in entities_layer.entityInstances
                    if "destructible" in e.tags_
                ],
                "buttons": buttons,
                **visualizations_subdata,
                "pickupables": pickupables,
                "harvestables": [
                    {
                        "pos": e.grid_,
                        "harvestable_id": e.field("harvestable").upper(),
                    }
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "harvestable"
                ],
                # "obstacles": obstacles,
                "smiths": [
                    {"pos": e.grid_}
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "smith"
                ],
                "quest_boards": [
                    e.grid_
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "quest_board"
                ],
                "stashes": [
                    e.grid_
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "stash"
                ],
                "cauldrons": [
                    e.grid_
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "cauldron"
                ],
                "greenhouses": [
                    e.grid_
                    for e in entities_layer.entityInstances
                    if e.identifier_ == "greenhouse"
                ],
                "decorations": decorations,
                "decoration_colliders": decoration_colliders,
            }
        )

    level_to_index = {
        level["identifier"]: i for i, level in enumerate(loaded_json["levels"])
    }

    return (
        {
            "levels": levels_data,
            **root_data,
        },
        level_to_index,
        narrative_points,
        gates_per_level_to_index,
    )


def do_ldtk_decorations():
    with open(ASSETS_DIR / "level.ldtk", encoding="utf-8") as in_file:
        level = json.load(in_file)

    level_entity_defs = level["defs"]["entities"]

    already_present_entities = []
    for e in level_entity_defs:
        identifier = e["identifier"]
        if identifier.startswith("d_"):
            already_present_entities.append(identifier)

    def next_uid():
        uid = level["nextUid"]
        level["nextUid"] += 1
        return uid

    run_command(
        "free-tex-packer-cli --project {} --output {}".format(
            ART_DIR / "atlas_decorations.ftpp", ART_DIR
        )
    )

    with open(ART_DIR / "atlas_decorations.json") as in_file:
        atlas_decorations_data = json.load(in_file)

    tileset_uid = 448

    for filepath in (ART_DIR / "decorations").rglob("*.png"):
        animated = False

        stem = filepath.stem

        if stem.startswith("a__"):
            animated = True

            if not stem.endswith("_0"):
                continue

            stem = stem.removeprefix("a__").removesuffix("_0")

        identifier = f"d_{stem}"

        if identifier not in already_present_entities:
            level_entity_defs.append(
                {
                    "identifier": identifier,
                    "uid": next_uid(),
                    "tags": ["decoration"],
                    "exportToToc": False,
                    "allowOutOfBounds": True,
                    "doc": None,
                    "width": 16,
                    "height": 16,
                    "resizableX": False,
                    "resizableY": False,
                    "minWidth": None,
                    "maxWidth": None,
                    "minHeight": None,
                    "maxHeight": None,
                    "keepAspectRatio": False,
                    "tileOpacity": 1,
                    "fillOpacity": 0,
                    "lineOpacity": 0,
                    "hollow": True,
                    "color": "#3E8948",
                    "renderMode": "Tile",
                    "showName": False,
                    "tilesetId": tileset_uid,
                    "tileRenderMode": "FullSizeUncropped",
                    "tileRect": {
                        "tilesetUid": tileset_uid,
                        "x": 0,
                        "y": 0,
                        "w": 16,
                        "h": 32,
                    },
                    "uiTileRect": None,
                    "nineSliceBorders": [],
                    "maxCount": 0,
                    "limitScope": "PerLevel",
                    "limitBehavior": "MoveLastOne",
                    "pivotX": 0,
                    "pivotY": 0,
                    "fieldDefs": [],
                }
            )

        for entity in level_entity_defs:
            if entity["identifier"] != identifier:
                continue

            if animated:
                frame = atlas_decorations_data["frames"][f"tiles/a__{stem}_0"]["frame"]
            else:
                frame = atlas_decorations_data["frames"][f"tiles/{stem}"]["frame"]

            entity["tilesetId"] = tileset_uid
            entity["tileRect"] = {
                "tilesetUid": tileset_uid,
                "x": frame["x"],
                "y": frame["y"],
                "w": frame["w"],
                "h": frame["h"],
            }
            entity["width"] = frame["w"]
            entity["height"] = frame["h"]

    with open(ASSETS_DIR / "level.ldtk", "w", encoding="utf-8") as out_file:
        json.dump(level, out_file)
