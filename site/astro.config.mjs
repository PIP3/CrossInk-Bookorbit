import { defineConfig } from 'astro/config';

function rewriteMarkdownLinks() {
  return (tree) => {
    visit(tree, (node) => {
      if (node.type !== 'link' && node.type !== 'image') {
        return;
      }

      if (typeof node.url !== 'string') {
        return;
      }

      node.url = node.url
        .replace(/^(\.\.\/)+USER_GUIDE\.md(#.*)?$/i, 'https://github.com/uxjulia/CrossInk/blob/main/USER_GUIDE.md$2')
        .replace(/^(\.\.\/)+SCOPE\.md(#.*)?$/i, 'https://github.com/uxjulia/CrossInk/blob/main/SCOPE.md$2')
        .replace(/^(\.\.\/)+GOVERNANCE\.md(#.*)?$/i, 'https://github.com/uxjulia/CrossInk/blob/main/GOVERNANCE.md$2')
        .replace(/(^|\/)README\.md(#.*)?$/i, '$1index.html$2')
        .replace(/\.md(#.*)?$/i, '.html$1');
    });
  };
}

function visit(node, visitor) {
  visitor(node);
  if (!Array.isArray(node.children)) {
    return;
  }
  for (const child of node.children) {
    visit(child, visitor);
  }
}

export default defineConfig({
  build: {
    format: 'file'
  },
  markdown: {
    remarkPlugins: [rewriteMarkdownLinks]
  },
  site: 'https://crossink.uxj.io'
});
