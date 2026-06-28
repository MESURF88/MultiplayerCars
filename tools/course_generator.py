#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


DEFAULT_REPO_ROOT = Path(__file__).resolve().parents[1]


def wall(x, z, width, depth, color="808080"):
    return {
        "x": float(x),
        "z": float(z),
        "width": float(width),
        "depth": float(depth),
        "color": color,
    }


def build_simple_circuit():
    outer_min_x = -34.0
    outer_max_x = 34.0
    outer_min_z = -22.0
    outer_max_z = 22.0
    inner_min_x = -16.0
    inner_max_x = 16.0
    inner_min_z = -8.0
    inner_max_z = 8.0
    thickness = 1.0

    return {
        "courseId": "simple-circuit",
        "lapCount": 10,
        "start": {
            "position": {"x": 2.0, "z": -14.0},
            "angle": 0.0,
        },
        "startLine": {
            "center": {"x": 0.0, "z": -14.0},
            "width": 1.0,
            "depth": 12.0,
            "forwardAngle": 0.0,
            "checkerCount": 14,
        },
        "lapCheckpoint": {
            "center": {"x": 0.0, "z": 14.0},
            "width": 1.0,
            "depth": 12.0,
        },
        "walls": [
            wall(outer_min_x - thickness, outer_min_z - thickness, (outer_max_x - outer_min_x) + thickness * 2.0, thickness),
            wall(outer_min_x - thickness, outer_max_z, (outer_max_x - outer_min_x) + thickness * 2.0, thickness),
            wall(outer_min_x - thickness, outer_min_z, thickness, outer_max_z - outer_min_z),
            wall(outer_max_x, outer_min_z, thickness, outer_max_z - outer_min_z),
            wall(inner_min_x, inner_min_z, inner_max_x - inner_min_x, thickness),
            wall(inner_min_x, inner_max_z - thickness, inner_max_x - inner_min_x, thickness),
            wall(inner_min_x, inner_min_z, thickness, inner_max_z - inner_min_z),
            wall(inner_max_x - thickness, inner_min_z, thickness, inner_max_z - inner_min_z),
        ],
    }


def course_bounds(course):
    min_x = min(w["x"] for w in course["walls"])
    min_z = min(w["z"] for w in course["walls"])
    max_x = max(w["x"] + w["width"] for w in course["walls"])
    max_z = max(w["z"] + w["depth"] for w in course["walls"])
    return min_x, min_z, max_x, max_z


def write_svg(course, path):
    min_x, min_z, max_x, max_z = course_bounds(course)
    padding = 4.0
    min_x -= padding
    min_z -= padding
    max_x += padding
    max_z += padding

    scale = 10.0
    width = (max_x - min_x) * scale
    height = (max_z - min_z) * scale

    def sx(x):
        return (x - min_x) * scale

    def sz(z):
        return (z - min_z) * scale

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width:.0f}" height="{height:.0f}" viewBox="0 0 {width:.0f} {height:.0f}">',
        '<rect width="100%" height="100%" fill="#1c8b52"/>',
    ]
    for course_wall in course["walls"]:
        lines.append(
            f'<rect x="{sx(course_wall["x"]):.2f}" y="{sz(course_wall["z"]):.2f}" '
            f'width="{course_wall["width"] * scale:.2f}" height="{course_wall["depth"] * scale:.2f}" '
            'fill="#808080" stroke="#595959" stroke-width="1"/>'
        )

    start_line = course["startLine"]
    lines.append(
        f'<rect x="{sx(start_line["center"]["x"] - start_line["width"] / 2.0):.2f}" '
        f'y="{sz(start_line["center"]["z"] - start_line["depth"] / 2.0):.2f}" '
        f'width="{start_line["width"] * scale:.2f}" height="{start_line["depth"] * scale:.2f}" '
        'fill="#ffffff" stroke="#000000" stroke-width="1"/>'
    )

    checkpoint = course["lapCheckpoint"]
    lines.append(
        f'<rect x="{sx(checkpoint["center"]["x"] - checkpoint["width"] / 2.0):.2f}" '
        f'y="{sz(checkpoint["center"]["z"] - checkpoint["depth"] / 2.0):.2f}" '
        f'width="{checkpoint["width"] * scale:.2f}" height="{checkpoint["depth"] * scale:.2f}" '
        'fill="#00e430" opacity="0.55"/>'
    )

    start = course["start"]["position"]
    lines.append(f'<circle cx="{sx(start["x"]):.2f}" cy="{sz(start["z"]):.2f}" r="4" fill="#fdf900"/>')
    lines.append("</svg>")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_baked_cpp(course_json, path):
    path.write_text(
        '#include "generatedCourses.hpp"\n\n'
        "const char* defaultCourseJson()\n"
        "{\n"
        "    return R\"COURSE_JSON(\n"
        f"{course_json}\n"
        ")COURSE_JSON\";\n"
        "}\n",
        encoding="utf-8",
    )


def main():
    parser = argparse.ArgumentParser(description="Generate a closed-loop course JSON, SVG preview, and baked C++ fallback.")
    parser.add_argument("--repo-root", type=Path, default=DEFAULT_REPO_ROOT)
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    course = build_simple_circuit()
    course_json = json.dumps(course, indent=2)

    course_dir = repo_root / "CPPClient" / "resources" / "courses"
    course_dir.mkdir(parents=True, exist_ok=True)
    (course_dir / "simple_circuit.json").write_text(course_json + "\n", encoding="utf-8")
    write_svg(course, course_dir / "simple_circuit.svg")
    write_baked_cpp(course_json, repo_root / "CPPClient" / "generatedCourses.cpp")

    print(f"Wrote {course_dir / 'simple_circuit.json'}")
    print(f"Wrote {course_dir / 'simple_circuit.svg'}")
    print(f"Wrote {repo_root / 'CPPClient' / 'generatedCourses.cpp'}")


if __name__ == "__main__":
    main()
