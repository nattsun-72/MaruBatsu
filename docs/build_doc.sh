#!/bin/bash
#==========================================================
# build_doc.sh
#   docs/parts/*.html を結合し、開発ガイドの単一HTMLを生成する。
#   行中の <!--CODE:相対パス--> を、実ソースをエスケープした
#   コードブロックへ置換する (ソース全文を常に最新で埋め込む)。
#
#   使い方:  bash docs/build_doc.sh
#   出力:    docs/guide.html
#==========================================================

# プロジェクトルートへ移動 (このスクリプトは docs/ にある)
cd "$(dirname "$0")/.." || exit 1

OUT_HTML="docs/guide.html"
PARTS_DIR="docs/parts"

#----------------------------------------------------------
# 1行ずつ読み、CODEプレースホルダを実ソースに置換して出力する
#----------------------------------------------------------
emit() {
    while IFS= read -r line || [ -n "$line" ]; do
        case "$line" in
            *'<!--CODE:'*)
                # <!--CODE:path--> から path を取り出す
                path="${line#*<!--CODE:}"
                path="${path%%-->*}"
                if [ -f "$path" ]; then
                    echo "<div class=\"codefile\">"
                    echo "<div class=\"codefile-name\">${path}</div>"
                    echo "<pre class=\"code\">"
                    # CR除去 → HTMLエスケープ (& を最初に処理)
                    sed -e 's/\r$//' \
                        -e 's/&/\&amp;/g' \
                        -e 's/</\&lt;/g' \
                        -e 's/>/\&gt;/g' "$path"
                    echo "</pre></div>"
                else
                    echo "<p class=\"warn\">[ソース未検出: ${path}]</p>"
                fi
                ;;
            *)
                echo "$line"
                ;;
        esac
    done
    return 0
}

#----------------------------------------------------------
# parts/*.html を名前順に結合
#----------------------------------------------------------
: > "$OUT_HTML"
for f in "$PARTS_DIR"/*.html; do
    [ -f "$f" ] || continue
    emit < "$f" >> "$OUT_HTML"
done

echo "生成完了: $OUT_HTML ($(wc -c < "$OUT_HTML") bytes)"

#----------------------------------------------------------
# Microsoft Edge のヘッドレス印刷でPDF化する
#   Edgeが見つからない場合は、guide.html を手動でPDF印刷すればよい。
#----------------------------------------------------------
EDGE="/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe"
ROOT_WIN="$(cygpath -m "$(pwd)" 2>/dev/null || pwd)"
if [ -f "$EDGE" ]; then
    "$EDGE" --headless --disable-gpu --no-pdf-header-footer \
        --print-to-pdf="${ROOT_WIN}/docs/guide.pdf" \
        "file:///${ROOT_WIN}/docs/guide.html" 2>/dev/null
    # 成果物を分かりやすい日本語名へ複製 (Edgeへは渡さずFS操作のみ)
    cp docs/guide.pdf "docs/MaruBatsu_開発ガイド.pdf"
    echo "生成完了: docs/MaruBatsu_開発ガイド.pdf ($(wc -c < docs/guide.pdf) bytes)"
else
    echo "Edge未検出: docs/guide.html をブラウザで開き、PDF印刷してください"
fi
