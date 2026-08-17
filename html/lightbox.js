/* QuiteRSS reader lightbox: click article images to view them full-size.
 * Self-contained, no external dependencies. Uses event delegation so it
 * also works for images inserted later (newspaper layout).
 */
(function () {
  "use strict";

  if (document.getElementById("qr-lightbox")) return;

  var CSS =
    "#qr-lightbox{display:none;position:fixed;top:0;left:0;right:0;bottom:0;" +
    "z-index:2147483000;background:rgba(0,0,0,0.88);cursor:zoom-out;" +
    "text-align:center;overflow:auto;-webkit-user-select:none;}" +
    "#qr-lightbox img{max-width:94%;max-height:92vh;margin-top:4vh;" +
    "box-shadow:0 8px 40px rgba(0,0,0,0.7);border-radius:4px;background:#fff;}" +
    "#qr-lightbox .qr-lb-caption{color:#eee;font:13px/1.5 sans-serif;padding:12px;" +
    "max-width:94%;margin:0 auto;word-break:break-word;}" +
    "#qr-lightbox .qr-lb-hint{color:#aaa;font:12px/1.5 sans-serif;padding:4px 0 14px;}";

  var style = document.createElement("style");
  style.textContent = CSS;
  document.head.appendChild(style);

  var ov = document.createElement("div");
  ov.id = "qr-lightbox";
  var img = document.createElement("img");
  img.alt = "";
  img.src = "";
  var caption = document.createElement("div");
  caption.className = "qr-lb-caption";
  var hint = document.createElement("div");
  hint.className = "qr-lb-hint";
  hint.textContent = "ESC";
  ov.appendChild(img);
  ov.appendChild(caption);
  ov.appendChild(hint);
  document.body.appendChild(ov);

  function close() {
    ov.style.display = "none";
    img.src = "";
  }

  ov.addEventListener("click", function (e) {
    if (e.target === ov) close();
  });

  document.addEventListener("keydown", function (e) {
    if (e.key === "Escape" || e.key === "Esc") close();
  });

  function isArticleImage(t) {
    if (!t || t.tagName !== "IMG") return false;
    if (t.classList.contains("quiterss-img")) return false; // app UI icons
    if (t.closest(".newsTable")) return true;               // article body
    if (t.classList.contains("enclosureImg")) return true;  // enclosure image
    return false;
  }

  document.addEventListener("click", function (e) {
    var t = e.target;
    if (!isArticleImage(t)) return;
    e.preventDefault();
    e.stopPropagation();
    var src = t.currentSrc || t.src;
    if (!src) return;
    img.src = src;
    caption.textContent = t.alt || t.title || "";
    ov.style.display = "block";
  }, true);
})();
