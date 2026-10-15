import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

EXPECTED_SHA = "c6affc88cfd4da21f2436dfece2012ed5b1fe457bdfd38e170ccf7a7a1974a3d"
EXPECTED_SIZE = 6634200
TAG = "v0.2.1-rotate"
SOURCE = "3a8580e6d691880bf246eb13fe2f8798ea945c78"
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
        "name": "动漫共和国 v0.2.1 · 横竖屏切换",
        "prerelease": True,
        "body": "新增阅读界面的横屏/竖屏切换：按 − 键切换，方向自动保存。竖屏画面顺时针旋转 90°，可将 Switch 逆时针竖持阅读。\n\n"
                "工具栏随画面旋转；方向键与右摇杆按竖持方向映射。切换保留当前页，重置缩放并重新适配画幅，不重新下载图片。按右摇杆可重试加载。\n\n"
                "独立 NSP 内置证书、接口配置和用户 LOGO，无需额外 config.ini 或 cacert.pem。\n\n"
                "Switch 原生构建、桌面构建、旋转及翻页/续读界面测试、安装包结构校验通过。尚需 Switch 真机验证，上游接口偶有 502。\n\n"
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
