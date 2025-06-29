# ruff: noqa: ERA001

from array import array
from collections import deque
from dataclasses import dataclass
from math import cos, sin
from pathlib import Path
from random import randint

import cv2
import numpy as np
from bf_lib import (
    ART_DIR,
    PIXEL_SCALE_MULTIPLIER,
    TEMP_DIR,
    recursive_mkdir,
    run_command,
)
from PIL import Image

BASE_SCALE = 5
OUTLINE_DISTANCE = 5
SHADOW_DISTANCE = 20
SHADOW_ALPHA = 140


# ------------------------------------------------------------------------------
# Rought 'cut' outline.
# ------------------------------------------------------------------------------

DEBUG = 0
SVG_STYLE = 'style="background-color:#ff3400;"' if DEBUG else ""

OFF_ = 0.4
MIN_OUTLINE_OFFSET = -OFF_
MAX_OUTLINE_OFFSET = OFF_

SVG_OFFSET = 100
SHADOW_D = "0.2"

SVG_TEMPLATE = rf"""<?xml version="1.0" encoding="utf-8"?>
<svg
    baseProfile="full"
    height="100%"
    version="1.1"
    width="100%"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:ev="http://www.w3.org/2001/xml-events"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    {{}}
>
    <defs>
        <filter id="filter">
            <feDropShadow
                dx="{SHADOW_D}"
                dy="{SHADOW_D}"
                stdDeviation="5"
                flood-color="black"
                flood-opacity="0.25"
            />
            <feDropShadow
                dx="{SHADOW_D}"
                dy="-{SHADOW_D}"
                stdDeviation="5"
                flood-color="black"
                flood-opacity="0.25"
            />
            <feDropShadow
                dx="-{SHADOW_D}"
                dy="-{SHADOW_D}"
                stdDeviation="5"
                flood-color="black"
                flood-opacity="0.25"
            />
            <feDropShadow
                dx="-{SHADOW_D}"
                dy="{SHADOW_D}"
                stdDeviation="5"
                flood-color="black"
                flood-opacity="0.25"
            />
        </filter>
    </defs>
    {{}}
    <image
        x="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
        y="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
        width="{{}}"
        height="{{}}"
        image-rendering="optimizeSpeed"
        xlink:href="{{}}"
    />
</svg>
"""

SVG_SHADOW_TEMPLATE = rf"""<?xml version="1.0" encoding="utf-8"?>
<svg
    baseProfile="full"
    height="100%"
    version="1.1"
    width="100%"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:ev="http://www.w3.org/2001/xml-events"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    {{}}
>
    <defs>
        <filter id="filter">
            <feDropShadow
                dx="0"
                dy="0"
                stdDeviation="3"
                flood-color="black"
                flood-opacity="1.00"
            />
        </filter>
    </defs>
    {{}}
    <mask>
        <image
            x="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
            y="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
            width="{{}}"
            height="{{}}"
            image-rendering="optimizeSpeed"
            xlink:href="{{}}"
        />
    </mask>
</svg>
"""

SVG_OUTLINE_TEMPLATE = rf"""<?xml version="1.0" encoding="utf-8"?>
<svg
    baseProfile="full"
    version="1.1"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:ev="http://www.w3.org/2001/xml-events"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    {{}}
>
    <defs>
        <filter id="filter">
            <feDropShadow
                dx="0"
                dy="0"
                stdDeviation="10"
                flood-color="black"
                flood-opacity="1.0"
            />
        </filter>
    </defs>
    {{}}
    <image
        x="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
        y="{SVG_OFFSET * PIXEL_SCALE_MULTIPLIER}"
        width="{{}}"
        height="{{}}"
        image-rendering="optimizeSpeed"
        xlink:href="{{}}"
    />
</svg>
"""


# [0, 1)
def frand() -> float:
    return randint(0, 65535) / 65536


def lerp(a, b, t):
    return a + (b - a) * t


def frand_lerp(a, b):
    return lerp(a, b, frand())


@dataclass(frozen=True)
class Pos:
    __slots__ = ("x", "y")
    x: int
    y: int

    def __add__(self, v):
        assert isinstance(v, Pos)
        return Pos(self.x + v.x, self.y + v.y)

    def __sub__(self, v):
        assert isinstance(v, Pos)
        return Pos(self.x - v.x, self.y - v.y)

    def __lt__(self, v):
        assert isinstance(v, Pos)
        if self.x < v.x:
            return True
        if self.y < v.y:
            return True
        return False

    def __str__(self) -> str:
        return f"({self.x},{self.y})"


def is_in_bounds(pos: Pos, outer_size: Pos) -> bool:
    return pos.x >= 0 and pos.y >= 0 and pos.x < outer_size.x and pos.y < outer_size.y


def check_wall(walls, outer_size: Pos, pos: Pos) -> bool:
    if is_in_bounds(pos, outer_size):
        return walls[pos.y * outer_size.x + pos.x]

    return False


def bfs_mark_visited(visited, pos, size, walls):
    t = pos.y * size.x + pos.x
    assert not visited[t]
    assert walls[t]

    visited[t] = 1

    queue = deque([pos])
    while queue:
        poss = queue.popleft()

        for offset in (
            Pos(1, 0),
            Pos(0, 1),
            Pos(-1, 0),
            Pos(0, -1),
        ):
            p = poss + offset
            t1 = p.y * size.x + p.x

            if is_in_bounds(p, size) and not visited[t1] and walls[t1]:
                visited[t1] = 1
                queue.append(p)


def get_image_vertices(
    image: Image.Image, extrude_walls: int = 0
) -> tuple[tuple[Pos, ...], ...]:
    assert extrude_walls >= 0

    pixels = image.load()
    image_size = Pos(image.size[0], image.size[1])

    outer_size = Pos(
        image_size.x + (extrude_walls + 1) * 2, image_size.y + (extrude_walls + 1) * 2
    )
    outer_size_total_cells = outer_size.x * outer_size.y

    walls = array("b", (0 for _ in range(outer_size_total_cells)))
    offsets4 = [Pos(1, 0), Pos(0, -1), Pos(-1, 0), Pos(0, 1)]
    offsets8 = [
        Pos(1, 0),
        Pos(1, -1),
        Pos(0, -1),
        Pos(-1, -1),
        Pos(-1, 0),
        Pos(-1, 1),
        Pos(0, 1),
        Pos(1, 1),
    ]

    is_wall = lambda pos: check_wall(walls, outer_size, pos)

    for y in range(image_size.y):
        for x in range(image_size.x):
            wall = bool(pixels[x, y][3])
            walls[(y + extrude_walls + 1) * outer_size.x + x + extrude_walls + 1] = wall

    for _ in range(extrude_walls):
        prev_walls = walls
        walls = array("b", (0 for _ in range(outer_size_total_cells)))

        for y in range(1, outer_size.y - 1):
            for x in range(1, outer_size.x - 1):
                adjacent_wall = False

                for offset in offsets8:
                    new_pos = Pos(x, y) + offset
                    if prev_walls[new_pos.y * outer_size.x + new_pos.x]:
                        adjacent_wall = True
                        break

                if adjacent_wall:
                    walls[y * outer_size.x + x] = True

    island_first_wall_poses = []

    if True:
        visited = array("b", (0 for _ in range(outer_size_total_cells)))
        for y in range(outer_size.y):
            for x in range(outer_size.x):
                t = y * outer_size.x + x
                wall = walls[t]
                if wall and not visited[t]:
                    island_first_wall_poses.append(Pos(x, y))
                    bfs_mark_visited(visited, Pos(x, y), outer_size, walls)

    vertice_groups: list[list[Pos]] = []
    stride = outer_size.x

    direction_index_check_offsets = (Pos(0, -1), Pos(-1, -1), Pos(-1, 0), Pos(0, 0))

    for wpos in island_first_wall_poses:
        visited = array("b", (0 for _ in range(4 * outer_size_total_cells)))
        contour_walls = [(wpos, 0)]
        vertices = []

        while contour_walls:
            pos, direction_index = contour_walls.pop(0)

            visited[direction_index * outer_size_total_cells + pos.y * stride + pos.x] = (
                True
            )

            direction_cw = (direction_index + 3) % 4
            direction_ccw = (direction_index + 5) % 4

            off_straight = offsets4[direction_index]
            off_ccw = offsets4[direction_ccw]
            ccw_pos = pos + off_ccw
            straight_pos = pos + off_straight

            w1 = is_wall(pos + off_straight)
            w2 = is_wall(pos + off_ccw)
            w3 = is_wall(pos + off_straight + off_ccw)

            s = int(w1) + int(w2) + int(w3)
            if not w2 and (s == 0 or s == 2):
                v = pos + direction_index_check_offsets[direction_index]
                if not vertices or vertices[-1] != v:
                    vertices.append(v)

            ccw_vis = visited[
                direction_ccw * outer_size_total_cells + ccw_pos.y * stride + ccw_pos.x
            ]
            straight_vis = visited[
                direction_index * outer_size_total_cells
                + straight_pos.y * stride
                + straight_pos.x
            ]
            cw_vis = visited[
                direction_cw * outer_size_total_cells + pos.y * stride + pos.x
            ]
            if (not ccw_vis) and is_wall(ccw_pos):
                contour_walls.append((ccw_pos, direction_ccw))
            elif (not straight_vis) and is_wall(straight_pos):
                contour_walls.append((straight_pos, direction_index))
            elif not cw_vis:
                contour_walls.append((pos, direction_cw))

        vertice_groups.append(tuple(vertices))

    return tuple(vertice_groups)


def transform_vertices(vertices_: list[list[Pos]]) -> list[list[Pos]]:
    new_vertices = []
    for vertices in vertices_:
        new_vertices.append([])

        for i in range(len(vertices)):
            direction = frand() * 2 * 3.14159265358979
            dx = MAX_OUTLINE_OFFSET * cos(direction)
            dy = MAX_OUTLINE_OFFSET * sin(direction)

            v = vertices[i] + Pos(dx, dy)
            new_vertices[-1].append(v)

    return new_vertices


def convert_image(filepath: Path) -> None:
    output_directory = Path("src") / "assets" / "art" / "intermediate"
    d = filepath.relative_to(filepath.parent.parent).parent
    output_filepath = output_directory / d / (filepath.stem + ".png")

    recursive_mkdir(output_filepath)

    image = Image.open(filepath)
    if image.mode != "RGBA":
        image = image.convert("RGBA")

    EXTRUDE_WALLS = 1
    vertices = [
        [
            pos + Pos(SVG_OFFSET, SVG_OFFSET) - Pos(EXTRUDE_WALLS, EXTRUDE_WALLS)
            for pos in v
        ]
        for v in transform_vertices(get_image_vertices(image, EXTRUDE_WALLS))
    ]

    vertices_joined = [
        ", ".join(
            "{} {}".format(pos.x * PIXEL_SCALE_MULTIPLIER, pos.y * PIXEL_SCALE_MULTIPLIER)
            for pos in v
        )
        for v in vertices
    ]

    svg_vertices = "".join(
        """
<path d="M {} L {}" fill="white"
    style="filter:url(#filter);"
/>
""".format(
            "{} {}".format(
                vertices[i][0].x * PIXEL_SCALE_MULTIPLIER,
                vertices[i][0].y * PIXEL_SCALE_MULTIPLIER,
            ),
            vertices_joined[i],
        )
        for i in range(len(vertices))
    )

    default_svg = SVG_TEMPLATE.format(
        SVG_STYLE,
        svg_vertices,
        image.size[0] * PIXEL_SCALE_MULTIPLIER,
        image.size[1] * PIXEL_SCALE_MULTIPLIER,
        filepath.absolute(),
    )

    shadow_svg = SVG_SHADOW_TEMPLATE.format(
        SVG_STYLE,
        svg_vertices,
        image.size[0] * PIXEL_SCALE_MULTIPLIER,
        image.size[1] * PIXEL_SCALE_MULTIPLIER,
        filepath.absolute(),
    )

    outline_svg = SVG_OUTLINE_TEMPLATE.format(
        SVG_STYLE,
        svg_vertices,
        image.size[0] * PIXEL_SCALE_MULTIPLIER,
        image.size[1] * PIXEL_SCALE_MULTIPLIER,
        filepath.absolute(),
    )

    for svg_variant in zip(
        (default_svg, shadow_svg, outline_svg),
        (".default.png", ".shadow.png", ".outline.png"),
    ):
        run_command(
            [
                "resvg",
                "--export-area-drawing",
                "--resources-dir",
                # "--skip-system-fonts",
                ".",
                "-",
                str(output_filepath) + svg_variant[1],
            ],
            input=svg_variant[0],
        )


# ------------------------------------------------------------------------------
# Smooth outline.
# ------------------------------------------------------------------------------


def outline(
    *, image, stroke_size, color, is_shadow, threshold=0, blend_image_on_top=True
) -> Image:
    img = np.asarray(image)
    h, w, _ = img.shape
    padding = stroke_size
    alpha = img[:, :, 3]
    rgb_img = img[:, :, 0:3]
    bigger_img = cv2.copyMakeBorder(
        rgb_img,
        padding,
        padding,
        padding,
        padding,
        cv2.BORDER_CONSTANT,
        value=(0, 0, 0, 0),
    )
    alpha = cv2.copyMakeBorder(
        alpha, padding, padding, padding, padding, cv2.BORDER_CONSTANT, value=0
    )
    bigger_img = cv2.merge((bigger_img, alpha))
    h, w, _ = bigger_img.shape

    _, alpha_without_shadow = cv2.threshold(
        alpha, threshold, 255, cv2.THRESH_BINARY
    )  # threshold=0 in photoshop
    alpha_without_shadow = 255 - alpha_without_shadow
    dist = cv2.distanceTransform(
        alpha_without_shadow, cv2.DIST_L2, cv2.DIST_MASK_5
    )  # dist l1 : L1 , dist l2 : l2

    if is_shadow:
        stroked = change_matrix_shadow(dist, stroke_size)
    else:
        stroked = change_matrix_outline(dist, stroke_size)

    stroke_b = np.full((h, w), color[2], np.uint8)
    stroke_g = np.full((h, w), color[1], np.uint8)
    stroke_r = np.full((h, w), color[0], np.uint8)
    stroke_alpha = (stroked * color[3]).astype(np.uint8)

    stroke = cv2.merge((stroke_b, stroke_g, stroke_r, stroke_alpha))
    stroke = cv2pil(stroke)
    if blend_image_on_top:
        bigger_img = cv2pil(bigger_img)
        stroke = Image.alpha_composite(stroke, bigger_img)
    return stroke


def change_matrix_outline(input_mat, stroke_size):
    # stroke_size = stroke_size - 1
    mat = np.ones(input_mat.shape)
    check_size = stroke_size + 1.0
    mat[input_mat > check_size] = 0
    border = (input_mat > stroke_size) & (input_mat <= check_size)
    mat[border] = 1.0 - (input_mat[border] - stroke_size)
    return mat


def change_matrix_shadow(input_mat, stroke_size):
    mat = np.ones(input_mat.shape)
    mat[input_mat > stroke_size] = 0
    border = input_mat <= stroke_size

    # mat[border] = (stroke_size - input_mat[border]) / stroke_size

    # NOTE: Squared easing of shadow decay.
    mat[border] = (
        (stroke_size - input_mat[border])
        * (stroke_size - input_mat[border])
        / stroke_size
        / stroke_size
    )
    return mat


def cv2pil(cv_img):
    return Image.fromarray(cv_img.astype("uint8"))


OutlineType_DEFAULT = 0
OutlineType_JUST_SHADOW = 1
OutlineType_JUST_OUTLINE = 2
OutlineType_JUST_SCALE = 3


def make_smooth_outlined_version_of(filepath: Path, outline_types) -> None:
    output_directory = TEMP_DIR / "art"
    d = filepath.relative_to(filepath.parent.parent).parent

    image = Image.open(filepath)
    image = image.convert("RGBA")
    image = image.resize(
        (image.size[0] * BASE_SCALE, image.size[1] * BASE_SCALE), Image.NEAREST
    )

    if OutlineType_JUST_SCALE in outline_types:
        output_filepath = output_directory / d / (filepath.stem + "__scaled.png")
        recursive_mkdir(output_filepath)
        image.save(output_filepath)
        print("Saved", output_filepath)

    if OutlineType_DEFAULT in outline_types:
        outlined_image = outline(
            image=image,
            stroke_size=OUTLINE_DISTANCE,
            color=(255, 255, 255, 255),
            is_shadow=False,
            blend_image_on_top=True,
        )
        output_filepath = output_directory / d / (filepath.stem + ".png")
        recursive_mkdir(output_filepath)
        shadow_image = outline(
            image=outlined_image,
            stroke_size=SHADOW_DISTANCE,
            color=(0, 0, 0, SHADOW_ALPHA),
            is_shadow=True,
            blend_image_on_top=True,
        )
        shadow_image.save(output_filepath)
        print("Saved", output_filepath)

    if OutlineType_JUST_OUTLINE in outline_types:
        outlined_image = outline(
            image=image,
            stroke_size=OUTLINE_DISTANCE,
            color=(255, 255, 255, 255),
            is_shadow=False,
            blend_image_on_top=False,
        )
        output_filepath = output_directory / d / (filepath.stem + "__outlined.png")
        recursive_mkdir(output_filepath)
        outlined_image.save(output_filepath)
        print("Saved", output_filepath)

    if OutlineType_JUST_SHADOW in outline_types:
        outlined_image = outline(
            image=image,
            stroke_size=OUTLINE_DISTANCE,
            color=(255, 255, 255, 255),
            is_shadow=False,
            blend_image_on_top=False,
        )
        output_filepath = output_directory / d / (filepath.stem + "__shadowed.png")
        recursive_mkdir(output_filepath)
        default_image = outline(
            image=outlined_image,
            stroke_size=SHADOW_DISTANCE,
            color=(0, 0, 0, SHADOW_ALPHA),
            is_shadow=True,
            blend_image_on_top=False,
        )
        default_image.save(output_filepath)
        print("Saved", output_filepath)


def main() -> None:
    test()

    files: list[tuple[Path, tuple[int, ...]]] = [
        (filepath, (OutlineType_DEFAULT,))
        for filepath in (ART_DIR / "to_outline").rglob("*.png")
    ]
    files.extend(
        (
            Path(filepath),
            (OutlineType_JUST_SHADOW, OutlineType_JUST_OUTLINE, OutlineType_JUST_SCALE),
        )
        for filepath in (ART_DIR / "to_bone").rglob("*.png")
    )

    for filepath, outline_types in files:
        # convert_image(filepath)
        make_smooth_outlined_version_of(filepath, outline_types)


def test() -> None:
    for filepath, expected in (
        (
            "cli/tests/1.png",
            (
                (
                    Pos(5, 0),
                    Pos(5, 1),
                    Pos(4, 1),
                    Pos(4, 2),
                    Pos(5, 2),
                    Pos(5, 5),
                    Pos(4, 5),
                    Pos(4, 4),
                    Pos(1, 4),
                    Pos(1, 3),
                    Pos(0, 3),
                    Pos(0, 1),
                    Pos(1, 1),
                    Pos(1, 2),
                    Pos(3, 2),
                    Pos(3, 1),
                    Pos(2, 1),
                    Pos(2, 0),
                ),
            ),
        ),
        (
            "cli/tests/2.png",
            (
                (
                    Pos(1, 0),
                    Pos(1, 1),
                    Pos(0, 1),
                    Pos(0, 0),
                ),
            ),
        ),
        (
            "cli/tests/3.png",
            (
                (
                    Pos(4, 0),
                    Pos(4, 4),
                    Pos(0, 4),
                    Pos(0, 0),
                ),
            ),
        ),
        (
            "cli/tests/4.png",
            (
                (
                    Pos(1, 0),
                    Pos(1, 1),
                    Pos(0, 1),
                    Pos(0, 0),
                ),
                (
                    Pos(3, 0),
                    Pos(3, 1),
                    Pos(2, 1),
                    Pos(2, 0),
                ),
            ),
        ),
    ):
        result = get_image_vertices(Image.open(filepath))
        assert expected == result


if __name__ == "__main__":
    main()
