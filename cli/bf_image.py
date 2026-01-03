# Imports.  {  ###
from pathlib import Path
from typing import Any, Callable, TypeAlias

import cv2
import numpy as np
from bf_lib import ART_TEXTURES_DIR, log, recursive_mkdir
from PIL import Image, ImageChops, ImageDraw, ImageEnhance

# }

ConveyorDatum: TypeAlias = tuple[Image.Image, Path]
ConveyorCallable: TypeAlias = Callable[[Image.Image, Path], ConveyorDatum]


def _change_matrix_outline(input_mat, radius: int):
    # {  ###
    # radius = radius - 1
    mat = np.ones(input_mat.shape)
    check_size = radius + 1.0
    mat[input_mat > check_size] = 0
    border = (input_mat > radius) & (input_mat <= check_size)
    mat[border] = 1.0 - (input_mat[border] - radius)
    return mat
    # }


def _change_matrix_shadow(input_mat, radius: int):
    # {  ###
    mat = np.ones(input_mat.shape)
    mat[input_mat > radius] = 0
    border = input_mat <= radius

    # mat[border] = (radius - input_mat[border]) / radius

    # NOTE: Squared easing of shadow decay.
    mat[border] = (
        (radius - input_mat[border]) * (radius - input_mat[border]) / radius / radius
    )
    return mat
    # }


def _cv2pil(cv_img):
    return Image.fromarray(cv_img.astype("uint8"))


def outline(
    image: Image.Image,
    *,
    radius: int,
    color: tuple[int, int, int, int] | tuple[int, int, int] = (0, 0, 0, 255),
    is_shadow: bool = False,
    threshold: int = 0,
    blend_image_on_top: bool = True,
    extend: bool = True,
) -> Image.Image:
    # {  ###
    assert threshold >= 0

    if len(color) == 3:
        color = (*color, 255)

    img = np.asarray(image)

    h = img.shape[0]
    w = img.shape[1]

    padding = radius
    alpha = img[:, :, 3]
    rgb_img = img[:, :, 0:3]
    extend_padding = padding
    if not extend:
        extend_padding = 0
    bigger_img = cv2.copyMakeBorder(
        rgb_img,
        extend_padding,
        extend_padding,
        extend_padding,
        extend_padding,
        cv2.BORDER_CONSTANT,
        value=(0, 0, 0, 0),
    )
    alpha = cv2.copyMakeBorder(  #  type: ignore  # noqa
        alpha,
        extend_padding,
        extend_padding,
        extend_padding,
        extend_padding,
        cv2.BORDER_CONSTANT,
        value=0,
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
        stroked = _change_matrix_shadow(dist, radius)
    else:
        stroked = _change_matrix_outline(dist, radius)

    stroke_b = np.full((h, w), color[0], np.uint8)
    stroke_g = np.full((h, w), color[1], np.uint8)
    stroke_r = np.full((h, w), color[2], np.uint8)
    stroke_alpha = (stroked * color[3]).astype(np.uint8)

    stroke = cv2.merge((stroke_b, stroke_g, stroke_r, stroke_alpha))
    stroke = _cv2pil(stroke)
    if blend_image_on_top:
        bigger_img = _cv2pil(bigger_img)
        stroke = Image.alpha_composite(stroke, bigger_img)  # type: ignore  # noqa
    return stroke
    # }


def extract_white(grayscale_image: Image.Image) -> Image.Image:
    # {  ###
    img = np.asarray(grayscale_image)
    h, w, _ = img.shape
    one = np.full((h, w), 255, np.uint8)
    img_r = img[:, :, 0]
    img_alpha = img[:, :, 3]
    img_front = cv2.merge((one, one, one, cv2.min(img_r, img_alpha)))
    return _cv2pil(img_front)
    # }


def extract_black(grayscale_image: Image.Image) -> Image.Image:
    # {  ###
    img = np.asarray(grayscale_image)
    h, w, _ = img.shape
    one = np.full((h, w), 255, np.uint8)
    img_r = img[:, :, 0]
    img_alpha = img[:, :, 3]
    alpha = cv2.min(255 - img_r, img_alpha)
    out_img = cv2.merge((one, one, one, alpha))
    return _cv2pil(out_img)
    # }


def replace_color(image: Image.Image, color: tuple[int, int, int]) -> Image.Image:
    # {  ###
    img = np.asarray(image)
    h, w, _ = img.shape
    alpha = img[:, :, 3]
    out_img = cv2.merge(
        (
            np.full((h, w), color[0], np.uint8),
            np.full((h, w), color[1], np.uint8),
            np.full((h, w), color[2], np.uint8),
            alpha,
        )
    )
    return _cv2pil(out_img)
    # }


def conveyor_replace_color(color: tuple[int, int, int]) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        image = replace_color(image_, color)
        return image, path_

    return inner
    # }


red = lambda image: replace_color(image, (255, 0, 0))
green = lambda image: replace_color(image, (0, 255, 0))
blue = lambda image: replace_color(image, (0, 0, 255))
white = lambda image: replace_color(image, (255, 255, 255))
black = lambda image: replace_color(image, (0, 0, 0))

conveyor_red = conveyor_replace_color((255, 0, 0))
conveyor_green = conveyor_replace_color((0, 255, 0))
conveyor_blue = conveyor_replace_color((0, 0, 255))
conveyor_white = conveyor_replace_color((255, 255, 255))
conveyor_black = conveyor_replace_color((0, 0, 0))


def invert(image: Image.Image) -> Image.Image:
    return ImageChops.invert(image)


def remap(
    grayscale_image: Image.Image,
    black_to: tuple[int, int, int],
    white_to: tuple[int, int, int],
) -> Image.Image:
    # {  ###
    img = np.asarray(grayscale_image)
    img_r = img[:, :, 0]
    img_g = img[:, :, 1]
    img_b = img[:, :, 2]
    out_img = cv2.merge(
        (
            (255 - img_r) * (black_to[0] / 255) + (img_r * (white_to[0] / 255)),
            (255 - img_g) * (black_to[1] / 255) + (img_g * (white_to[1] / 255)),
            (255 - img_b) * (black_to[2] / 255) + (img_b * (white_to[2] / 255)),
            img[:, :, 3] * 1.0,
        )
    )
    return _cv2pil(out_img)
    # }


def remap_grayscale(
    grayscale_image: Image.Image, black_to: int, white_to: int
) -> Image.Image:
    # {  ###
    return remap(
        grayscale_image, (black_to, black_to, black_to), (white_to, white_to, white_to)
    )
    # }


def conveyor(
    folder_name: str,
    conveyor_name: str,
    *args: ConveyorCallable,
    out_dir: Path | str = ART_TEXTURES_DIR,
) -> None:
    # {  ###
    log.info(f"conveyor: `{folder_name}`: {conveyor_name}...")
    folder = ART_TEXTURES_DIR / folder_name
    assert folder.exists(), f"`{folder}` does not exist!"

    files = list(folder.glob("*.png"))
    for f in files:
        img: Image.Image = Image.open(f)
        for func in args:
            img2, f = func(img, f)  # noqa: PLW2901
            img = img2
        img.save(Path(out_dir) / f.name)
    log.info(f"conveyor: `{folder_name}`: {conveyor_name}... Success!")
    # }


def conveyor_prefix(prefix: str) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        extension = path_.name.rsplit(".", 1)[-1]
        path = path_.parent / "{}_{}.{}".format(prefix, path_.stem, extension)
        return image_, path

    return inner
    # }


def conveyor_suffix(suffix: str) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        extension = path_.name.rsplit(".", 1)[-1]
        path = path_.parent / "{}_{}.{}".format(path_.stem, suffix, extension)
        return image_, path

    return inner
    # }


def conveyor_scale(factor: float) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        image = image_.resize(
            (round(image_.size[0] * factor), round(image_.size[1] * factor))
        )
        return image, path_

    return inner
    # }


def conveyor_outline(**kwargs: Any) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        image = outline(image=image_, **kwargs)
        return image, path_

    return inner
    # }


def conveyor_remap(
    black_to: tuple[int, int, int], white_to: tuple[int, int, int]
) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        image = remap(image_, black_to, white_to)
        return image, path_

    return inner
    # }


def conveyor_extract_white() -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        return extract_white(image_), path_

    return inner
    # }


def conveyor_extract_black() -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        return extract_black(image_), path_

    return inner
    # }


def conveyor_brightness(factor: float) -> ConveyorCallable:
    # {  ###
    def inner(image_: Image.Image, path_: Path) -> ConveyorDatum:
        image = ImageEnhance.Brightness(image_).enhance(factor)
        return image, path_

    return inner
    # }


def _shape(
    func_name: str,
    size: int | tuple[int, int],
    *,
    radius: int = 0,
    fill: tuple[int, int, int, int] = (255, 255, 255, 255),
    outline: tuple[int, int, int, int] = (0, 0, 0, 255),
    width: int = 0,
) -> Image.Image:
    # {  ###
    if isinstance(size, int):
        size = (size, size)

    assert size[0] > radius * 2
    assert size[1] > radius * 2
    assert radius >= 0
    assert width >= 0

    original_size = size

    scale_to_smooth_later = 2
    size = (size[0] * scale_to_smooth_later, size[1] * scale_to_smooth_later)
    radius *= scale_to_smooth_later
    width *= scale_to_smooth_later

    image = Image.new("RGBA", size)
    d = ImageDraw.ImageDraw(image, "RGBA")

    kw = {}
    if func_name == "rounded_rectangle":
        kw["radius"] = radius
    else:
        assert radius == 0

    getattr(d, func_name)(xy=(0, 0, *size), fill=fill, outline=outline, width=width, **kw)

    return image.resize(original_size)
    # }


def rectangle(*args: Any, **kwargs: Any) -> Image.Image:
    return _shape("rounded_rectangle", *args, **kwargs)


def ellipse(*args: Any, **kwargs: Any) -> Image.Image:
    return _shape("ellipse", *args, **kwargs)


def spritesheetify(
    image_path: Path,
    *,
    origin: tuple[int, int] = (0, 0),
    cell_size: tuple[int, int] | int,
    size: tuple[int, int],
    gap: tuple[int, int] | int,
    out_dir: Path,
    out_filename_prefix: str = "",
    out_filenames: list[str] | None = None,
) -> None:
    # {  ###
    recursive_mkdir(out_dir)

    image = Image.open(image_path)
    X, Y = size

    assert isinstance(gap, (int, tuple))
    if isinstance(gap, int):
        gap = (gap, gap)

    assert isinstance(cell_size, (int, tuple))
    if isinstance(cell_size, int):
        cell_size = (cell_size, cell_size)

    log.info(f"spritesheetify: {image_path}...")

    for y in range(Y):
        for x in range(X):
            t = y * X + x

            if (out_filenames is not None) and (t >= len(out_filenames)):
                break

            x0 = origin[0] + (cell_size[0] + gap[0]) * x
            y0 = origin[1] + (cell_size[1] + gap[1]) * y
            img = image.crop((x0, y0, x0 + cell_size[0], y0 + cell_size[1]))
            bbox = img.getbbox()

            if bbox is None:
                if out_filenames:
                    assert False, (
                        "In named spritesheet (`{}`) found a cell ({}, {}) that's named but doesn't contain anything".format(
                            str(out_filenames)[:200], x, y
                        )
                    )
                break

            if out_filenames is None:
                filename = str(t + 1)
            else:
                filename = out_filenames[t]

            img2 = img.crop(bbox)
            img2.save(out_dir / (out_filename_prefix + filename + ".png"))

    log.info(f"spritesheetify: {image_path}... Success!")
    # }


def draw_on_top(
    background: Image.Image,
    overlay: Image.Image,
    overlay_color: tuple[int, int, int, int] = (255, 255, 255, 255),
) -> Image.Image:
    # {  ###
    assert (
        background.size[0] / background.size[1] - overlay.size[0] / overlay.size[1]
    ) <= 0.0001, "Aspect ratios of images must be the same!"

    arr = np.asarray(overlay.resize(background.size), dtype=np.float32)
    arr *= np.array(overlay_color, dtype=np.float32) / 255.0
    o = Image.fromarray(arr.clip(0, 255).astype("uint8"), "RGBA")

    return Image.alpha_composite(background.convert("RGBA"), o)
    # }


# def draw_banner_and_text_over_screenshot(
#     screenshot_path: Path | str,
#     text: str,
#     font: Path | str,
#     banner_path: Path | str,
#     banner_color: tuple[int, int, int],
# ) -> Image:
#     screenshot = Image.open(screenshot_path)
#     banner = Image.open(banner_path)


###
