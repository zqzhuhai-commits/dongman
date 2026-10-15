import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

EXPECTED_SHA = "d8baac30e9a4d3b536fe68c39b97d1df20f61b6bd2649ec93cb03d9a80469fae"
EXPECTED_SIZE = 6632152
TAG = "v0.2.0-comic"
SOURCE = "2c76d40176623a8c94a29865bcf55fbcc8c6890a"
repository = os.environ["GITHUB_REPOSITORY"]
assert repository == "zqzhuhai-commits/dongman"
data = b"".join(Path(f"release-upload/dongman.nsp.part{i:02}").read_bytes()
                for i in range(1, 4))
assert len(data) == EXPECTED_SIZE
assert hashlib.sha256(data).hexdigest() == EXPECTED_SHA
assert data[:4] == b"PFS0"


def request(url, payload=None, mime="application/json"):
    body = json.dumps(payload).encode() if isinstance(payload, dict) else payload
    headers = {"Authorization": "Bearer " + os.environ["GH_TOKEN"],
               "Accept": "application/vnd.github+json",
               "X-GitHub-Api-Version": "2022-11-28",
               "Content-Type": mime}
    req = urllib.request.Request(url, data=body, headers=headers)
    with urllib.request.urlopen(req, timeout=180) as response:
        return json.load(response)


base = "https://api.github.com/repos/" + repository
try:
    release = request(base + "/releases/tags/" + TAG)
except urllib.error.HTTPError as error:
    if error.code != 404:
        raise
    release = request(base + "/releases", {
        "tag_name": TAG, "target_commitish": SOURCE,
        "name": "动漫共和国 v0.2.0 · Switch 漫画阅读版",
        "prerelease": True,
        "body": "基于用户提供的 Android APK 制作的 Switch 漫画阅读版。\n\n"
                "包含推荐、新作、热门、校园、冒险、收藏与阅读历史；章节目录、手柄翻页、缩放、适宽、跨章节与自动续读。使用用户提供的 LOGO。\n\n"
                "独立 NSP 内置证书和客户端接口配置，无需额外 config.ini 或 cacert.pem。\n\n"
                "Switch 原生构建、桌面协议测试、完整阅读界面流程和 NSP 结构校验通过。已成功取得真实漫画目录及图片；上游接口存在间歇性 502，支持重试。仍需 Switch 真机安装与运行验证。\n\n"
                "源码提交：" + SOURCE + "。\n\nSHA-256: `" + EXPECTED_SHA + "`",
    })
upload = release["upload_url"].split("{")[0]
existing = {asset["name"]: asset for asset in release.get("assets", [])}
for name, body, mime in [
    ("dongman.nsp", data, "application/octet-stream"),
    ("SHA256SUMS.txt", (EXPECTED_SHA + "  dongman.nsp\n").encode(), "text/plain"),
]:
    if name in existing:
        assert existing[name]["size"] == len(body)
        if name == "dongman.nsp":
            assert existing[name].get("digest") == "sha256:" + EXPECTED_SHA
        continue
    asset = request(upload + "?" + urllib.parse.urlencode({"name": name}), body, mime)
    assert asset["size"] == len(body)
    print(asset["browser_download_url"])
print(release["html_url"])
