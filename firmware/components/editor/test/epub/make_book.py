"""Write the test books: argv[1] = output folder."""
import sys, zipfile, os
out = sys.argv[1]

def book(name, files, stored_mimetype=True):
    with zipfile.ZipFile(os.path.join(out, name), "w") as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/epub+zip")
        for path, data in files.items():
            z.writestr(path, data, compress_type=zipfile.ZIP_DEFLATED)

container = """<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/></rootfiles>
</container>"""

opf3 = """<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://www.idpf.org/2007/opf" version="3.0">
 <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
  <dc:title>The &amp; Test Book</dc:title>
 </metadata>
 <manifest>
  <item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>
  <item id="c1" href="text/ch%201.xhtml" media-type="application/xhtml+xml"/>
  <item id="c2" href="text/ch2.xhtml" media-type="application/xhtml+xml"/>
  <item id="c3" href="text/ch3.xhtml" media-type="application/xhtml+xml"/>
 </manifest>
 <spine><itemref idref="c1"/><itemref idref="c2"/><itemref idref="c3"/></spine>
</package>"""

nav = """<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops"><body>
<nav epub:type="landmarks"><ol><li><a href="text/ch3.xhtml">Wrong</a></li></ol></nav>
<nav epub:type="toc"><ol>
 <li><a href="text/ch%201.xhtml">One</a></li>
 <li><a href="text/ch2.xhtml#start">Two <span>parts</span></a></li>
</ol></nav></body></html>"""

ch1 = """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Skip me</title>
<style>p { color: red }</style></head>
<body>
<h1>Chapter   One</h1>
<p>It was a <em>dark</em> and <strong>stormy</strong>
   night&#8230; &ldquo;Quoted&rdquo; &amp; done.</p>
<p>Line one<br/>Line two</p>
<p><em>Whole italic sentence. </em>After.</p>
<blockquote><p>A quote.</p></blockquote>
<ul><li>First</li><li><p>Second</p></li>
 <li>Third<ol><li>Nested a</li><li>Nested b</li></ol></li></ul>
<hr/>
<p># not a heading, 2 * 3 = 6, snake_case, a*b, 1. not a list</p>
<p>1. Starts with a number</p>
<pre>  code   kept
  as is</pre>
<p>Text with <code>x*y</code> and <img src="a.png" alt="A map"/> image.</p>
<p>soft&#173;hyphen</p>
<script>var x = "<p>no</p>";</script>
<svg><text>hidden</text></svg>
</body></html>"""

ch2 = "<html><body><h2 id='start'>Two</h2><p>Second chapter.</p></body></html>"
ch3 = "<html><body><p>" + "Long text. " * 3000 + "</p></body></html>"

book("test.epub", {
    "META-INF/container.xml": container,
    "OEBPS/content.opf": opf3,
    "OEBPS/nav.xhtml": nav,
    "OEBPS/text/ch 1.xhtml": ch1,
    "OEBPS/text/ch2.xhtml": ch2,
    "OEBPS/text/ch3.xhtml": ch3,
})

# EPUB 2: NCX table of contents, no nav document.
opf2 = """<package xmlns="http://www.idpf.org/2007/opf" version="2.0">
 <metadata><dc:title xmlns:dc="http://purl.org/dc/elements/1.1/">Old Book</dc:title></metadata>
 <manifest>
  <item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>
  <item id="a" href="a.html" media-type="application/xhtml+xml"/>
 </manifest>
 <spine toc="ncx"><itemref idref="a"/></spine></package>"""
ncx = """<ncx><navMap><navPoint id="p1"><navLabel><text>Alpha</text></navLabel>
<content src="a.html#x"/></navPoint></navMap></ncx>"""
book("epub2.epub", {
    "META-INF/container.xml": container.replace("OEBPS/content.opf", "content.opf"),
    "content.opf": opf2,
    "toc.ncx": ncx,
    "a.html": "<html><body><p>Hello</p></body></html>",
})

with open(os.path.join(out, "notzip.epub"), "w") as f:
    f.write("plain text, not a zip")
