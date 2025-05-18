import { marked } from "marked";
import { convert as htmlToText } from "html-to-text";
import { writeFile, readFileSync } from "fs";

let markdown = readFileSync(process.cwd() + "/README.md").toString();

const html = marked(markdown);

const text = htmlToText(html, {
  formatters: {
    h1BlockFormatter: function (elem, walk, builder, formatOptions) {
      builder.openBlock({
        leadingLineBreaks: formatOptions.leadingLineBreaks || 1,
      });
      const textLength = elem.children[0].data
        ? elem.children[0].data.length
        : 10;
      builder.addInline("=".repeat(textLength));
      builder.addLineBreak();
      walk(elem.children, builder);
      builder.addLineBreak();
      builder.addInline("=".repeat(textLength));
      builder.closeBlock({
        trailingLineBreaks: formatOptions.trailingLineBreaks || 1,
      });
    },
    h2BlockFormatter: function (elem, walk, builder, formatOptions) {
      builder.openBlock({
        leadingLineBreaks: formatOptions.leadingLineBreaks || 1,
      });
      const textLength = elem.children[0].data
        ? elem.children[0].data.length
        : 10;
      walk(elem.children, builder);
      builder.addLineBreak();
      builder.addInline("=".repeat(textLength));
      builder.closeBlock({
        trailingLineBreaks: formatOptions.trailingLineBreaks || 1,
      });
    },
    h3BlockFormatter: function (elem, walk, builder, formatOptions) {
      builder.openBlock({
        leadingLineBreaks: formatOptions.leadingLineBreaks || 1,
      });
      const textLength = elem.children[0].data
        ? elem.children[0].data.length
        : 10;
      walk(elem.children, builder);
      builder.addLineBreak();
      builder.addInline("-".repeat(textLength));
      builder.closeBlock({
        trailingLineBreaks: formatOptions.trailingLineBreaks || 1,
      });
    },
    preBlockFormatter: function (elem, walk, builder, formatOptions) {
      builder.openBlock({
        isPre: true,
        leadingLineBreaks: formatOptions.leadingLineBreaks || 1,
      });
      walk(elem.children, builder);
      builder.closeBlock({
        trailingLineBreaks: formatOptions.trailingLineBreaks || 1,
      });
    },
    linkBlockFormatter: function (elem, walk, builder, formatOptions) {
      let href = elem.attribs.href;
      href = href.replaceAll("mailto:", "");
      if (href.startsWith("/dist")) {
        href = null;
      }
      else if (href.startsWith("/")) {
        href = "https://github.com/codeflorist/dunedynasty/blob/master" + href;
      }
      else if (href.startsWith("#")) {
        href = "see below";
      }
      const text = elem.children[0].data;
      if (href === text || href === null) {
        builder.addInline(text);
      }
      else if (!text && elem.children[0].children) {
        walk(elem.children, builder);
        builder.addInline(" [" + href + "]");
      }
      else if (!text) {
        builder.addInline(href);
      }
      else {
        builder.addInline(text + " [" + href + "]");
      }
    },
    imgBlockFormatter: function (elem, walk, builder, formatOptions) {
      let src = elem.attribs.src;
      if (src.startsWith("/")) {
        src = "https://github.com/codeflorist/dunedynasty/blob/master" + src;
      }
      builder.addInline(src);
    },
  },
  selectors: [
    {
      selector: "h1",
      format: "h1BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "h2",
      format: "h2BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "h3",
      format: "h3BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "h4",
      format: "h3BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "h5",
      format: "h3BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "h6",
      format: "h3BlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "pre",
      format: "preBlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 0 },
    },
    {
      selector: "a",
      format: "linkBlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
    {
      selector: "img",
      format: "imgBlockFormatter",
      options: { leadingLineBreaks: 1, trailingLineBreaks: 2 },
    },
  ],
});

writeFile("README.txt", text, function (err) {
  if (err) {
    return console.log(err);
  }
  console.log("README.txt was saved!");
});
