"""Relative links in the top-level docs resolve (files, folders, #anchors into markdown)."""
import pathlib, re, pytest
R = pathlib.Path(__file__).resolve().parents[1]

def anchors(md: pathlib.Path):
    return {re.sub(r"[^\w\- ]", "", h.lower()).strip().replace(" ", "-") for h in re.findall(r"^#+\s+(.*)$", md.read_text(), re.M)}

@pytest.mark.parametrize("doc", ["README.md", "contracts/PROTOCOL.md"])
def test_relative_links_resolve(doc):
    src = R / doc; bad = []
    for link in re.findall(r"\]\(([^)\s]+)\)", src.read_text()):
        if link.startswith(("http://", "https://", "mailto:")): continue
        path, _, frag = link.partition("#")
        target = (src.parent / path) if path else src
        if not target.exists() or (frag and target.suffix == ".md" and frag not in anchors(target)): bad.append(link)
    assert bad == []
