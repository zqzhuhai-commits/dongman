import json
import os
import urllib.request

repo = os.environ["GITHUB_REPOSITORY"]
assert repo == "zqzhuhai-commits/dongman"
base = "https://api.github.com/repos/" + repo
headers = {
    "Authorization": "Bearer " + os.environ["GH_TOKEN"],
    "Accept": "application/vnd.github+json",
    "Content-Type": "application/json",
    "X-GitHub-Api-Version": "2022-11-28",
}
def request(url, payload=None):
    data = json.dumps(payload).encode() if payload is not None else None
    req = urllib.request.Request(url, data=data, headers=headers,
                                 method="PATCH" if data is not None else "GET")
    with urllib.request.urlopen(req, timeout=60) as response:
        return json.load(response)

release = request(base + "/releases/tags/v0.2.2-fontfix")
assert release["id"] == 413507796
assert release["target_commitish"] == "677558b4fe0da5580a20c3979fad48964f4082a3"
asset = next(a for a in release["assets"] if a["name"] == "dongman.nsp")
assert asset["size"] == 6634200
assert asset["digest"] == "sha256:1b4bc98829f390a38d939e4e154d2bbd189d81ec75eb3b973a2164475c496fcd"
published = request(base + "/releases/" + str(release["id"]),
                    {"prerelease": False, "draft": False, "make_latest": "true"})
assert not published["prerelease"] and not published["draft"]
print(published["html_url"])
