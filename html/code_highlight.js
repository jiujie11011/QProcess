/* Lightweight offline syntax highlighter for Quill (S-8)
 * Auto-detects language from code content and highlights common
 * tokens without any external dependency.
 */
(function () {
  "use strict";

  function esc(s) {
    return s.replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;");
  }

  function guessLang(code) {
    var s = code.slice(0, 3000);
    if (/#!.*(python|perl|ruby)/i.test(s) ||
        /\b(def|class|import|from|return|lambda|elif|print|None|True|False)\b/.test(s))
      return "python";
    if (/\b(function|var|const|let|=>|document\.|window\.|console\.|require)\b/.test(s))
      return "javascript";
    if (/<\?xml|<html|<div|<span|<body|<table|<!DOCTYPE/i.test(s))
      return "xml";
    if (/\b(public|private|protected|System\.out|new Scanner|String\[\])\b/.test(s))
      return "java";
    if (/#include\s*[<"]|std::|int main|printf|malloc|cout\s*<</.test(s))
      return "cpp";
    if (/\b(SELECT|INSERT|UPDATE|DELETE|FROM|WHERE|JOIN|CREATE|ALTER|TABLE)\b/i.test(s))
      return "sql";
    if (/\b(select|insert|update|delete|from|where|join|create|alter)\b/i.test(s))
      return "sql";
    if (/\b(int|char|float|double|void|struct|typedef|printf|scanf)\b/.test(s))
      return "c";
    if (/^\s*(sudo|apt|git|npm|pip|docker|cd|ls|rm|grep|tar|yarn)\b/m.test(s))
      return "bash";
    return "plain";
  }

  var langPatterns = {
    python: [
      [/#[^\n]*/, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:def|class|import|from|return|if|elif|else|for|while|lambda|with|as|try|except|raise|pass|yield|global|None|True|False|not|and|or|in|is)\b/, "kw"],
      [/\b\d+\.?\d*(?:[eE][+-]?\d+)?\b/, "nu"]
    ],
    javascript: [
      [/\/\/[^\n]*|\/\*[\s\S]*?\*\//, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|`(?:[^`\\]|\\.)*`)/, "st"],
      [/\b(?:var|let|const|function|return|if|else|for|while|do|class|new|typeof|instanceof|true|false|null|undefined|this|async|await|import|export|from)\b/, "kw"],
      [/\b\d+\.?\d*(?:[eE][+-]?\d+)?\b/, "nu"]
    ],
    java: [
      [/\/\/[^\n]*|\/\*[\s\S]*?\*\//, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:public|private|protected|static|final|class|interface|extends|implements|new|return|if|else|for|while|switch|case|break|void|int|boolean|String|double|float|long|try|catch|finally|throw|import|package|this|super|null|true|false)\b/, "kw"],
      [/\b\d+\.?\d*(?:[eE][+-]?\d+)?\b/, "nu"]
    ],
    c: [
      [/\/\/[^\n]*|\/\*[\s\S]*?\*\//, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:int|char|float|double|void|long|short|unsigned|signed|struct|typedef|union|enum|return|if|else|for|while|do|switch|case|break|continue|sizeof|static|const|goto|include|define|printf|scanf|malloc|free|NULL)\b/, "kw"],
      [/\b\d+\.?\d*(?:[eE][+-]?\d+)?\b|0x[0-9a-fA-F]+/, "nu"]
    ],
    cpp: [
      [/\/\/[^\n]*|\/\*[\s\S]*?\*\//, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:class|struct|public|private|protected|namespace|using|template|typename|virtual|override|const|static|inline|new|delete|return|if|else|for|while|do|switch|case|break|continue|int|char|float|double|void|bool|string|vector|map|auto|include|define|std::|cout|cin|endl|printf|NULL)\b/, "kw"],
      [/\b\d+\.?\d*(?:[eE][+-]?\d+)?\b|0x[0-9a-fA-F]+/, "nu"]
    ],
    sql: [
      [/--[^\n]*|\/\*[\s\S]*?\*\//, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:SELECT|INSERT|UPDATE|DELETE|FROM|WHERE|JOIN|LEFT|RIGHT|INNER|OUTER|ON|GROUP|BY|ORDER|HAVING|LIMIT|OFFSET|CREATE|ALTER|DROP|TABLE|INDEX|VIEW|PRIMARY|KEY|FOREIGN|REFERENCES|DISTINCT|AS|IN|NOT|NULL|AND|OR|BETWEEN|LIKE|UNION|ALL|COUNT|SUM|AVG|MIN|MAX|ASC|DESC)\b/i, "kw"],
      [/\b\d+\.?\d*\b/, "nu"]
    ],
    xml: [
      [/<!--[\s\S]*?-->/, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/<\/?[a-zA-Z][\w.-]*|\/?>/, "kw"],
      [/\b\d+\.?\d*\b/, "nu"]
    ],
    bash: [
      [/#[^\n]*/, "cm"],
      [/(?:"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')/, "st"],
      [/\b(?:if|then|else|elif|fi|for|while|do|done|function|return|local|export|case|esac|in|true|false)\b/, "kw"],
      [/\b\d+\.?\d*\b/, "nu"]
    ],
    plain: []
  };

  function highlightCode(code, lang) {
    var patterns = langPatterns[lang] || [];
    var matches = [];
    for (var p = 0; p < patterns.length; p++) {
      var src = patterns[p][0], cls = patterns[p][1];
      var re = new RegExp(src.source, "g");
      var m;
      while ((m = re.exec(code)) !== null) {
        matches.push({ s: m.index, e: m.index + m[0].length, c: cls, t: m[0] });
        if (m[0].length === 0) re.lastIndex++;
      }
    }
    matches.sort(function (a, b) { return a.s - b.s || a.e - b.e; });
    var out = "", pos = 0;
    for (var i = 0; i < matches.length; i++) {
      var mt = matches[i];
      if (mt.s < pos) continue;
      out += esc(code.substring(pos, mt.s));
      out += '<span class="tok-' + mt.c + '">' + esc(mt.t) + "</span>";
      pos = mt.e;
    }
    out += esc(code.substring(pos));
    return out;
  }

  function run() {
    var els = document.querySelectorAll("pre, code");
    for (var i = 0; i < els.length; i++) {
      var el = els[i];
      if (el.classList.contains("code-hl")) continue;
      var code = el.textContent || "";
      if (!code.trim()) continue;
      var lang = guessLang(code);
      el.innerHTML = highlightCode(code, lang);
      el.classList.add("code-hl");
      el.classList.add("code-" + lang);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", run);
  } else {
    run();
  }
})();
