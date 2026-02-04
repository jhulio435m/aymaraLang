const vscode = require('vscode');

const keywordItems = [
  { label: 'qallta', detail: 'Inicio de programa' },
  { label: 'tukuya', detail: 'Fin de programa' },
  { label: 'yatiya', detail: 'Declaración de variable' },
  { label: 'qillqa', detail: 'Imprimir salida' },
  { label: 'suti', detail: 'Condicional (alias de jisa)' },
  { label: 'jisa', detail: 'Condicional if' },
  { label: 'jani', detail: 'Condicional else' },
  { label: 'maysatxa', detail: 'Condicional else (alias)' },
  { label: 'kunawsati', detail: 'Bucle while' },
  { label: 'ukhakamaxa', detail: 'Bucle while (alias)' },
  { label: "sapüru", detail: 'Bucle for' },
  { label: 'taki', detail: 'Bucle for (alias)' },
  { label: "p'akhiña", detail: 'Salir de bucle' },
  { label: 'sarantaña', detail: 'Continuar bucle' },
  { label: 'lurawi', detail: 'Definir función' },
  { label: 'kuttaya', detail: 'Retornar valor' },
  { label: 'apnaq', detail: 'Importar módulo' },
  { label: "yant'aña", detail: 'Bloque try' },
  { label: 'katjaña', detail: 'Bloque catch' },
  { label: 'tukuyawi', detail: 'Bloque finally' },
  { label: 'pantja', detail: 'Lanzar error' },
  { label: 'kasta', detail: 'Declarar clase' },
  { label: 'machaqa', detail: 'Crear instancia (new)' },
  { label: 'aka', detail: 'Referencia this' },
  { label: 'jila', detail: 'Herencia (extends)' },
  { label: 'jikxata', detail: 'Sobrescribir método' },
  { label: 'sapa', detail: 'Modificador privado' },
  { label: 'taqi', detail: 'Modificador público' },
  { label: 'sapakasta', detail: 'Modificador static' },
  { label: "uñt'aya", detail: 'Getter' },
  { label: 'chura', detail: 'Setter' },
  { label: 'jilaaka', detail: 'Referencia super' }
];

const typeItems = [
  { label: 'jakhüwi', detail: 'Tipo número' },
  { label: 'aru', detail: 'Tipo cadena' },
  { label: "listaña", detail: 'Tipo lista' },
  { label: "t'aqa", detail: 'Tipo lista (alias)' },
  { label: 'mapa', detail: 'Tipo mapa' }
];

const literalItems = [
  { label: 'utji', detail: 'Literal verdadero (true)' },
  { label: 'chiqa', detail: 'Literal verdadero (alias)' },
  { label: 'janiutji', detail: 'Literal falso (false)' },
  { label: "k'ari", detail: 'Literal falso (alias)' }
];

const snippetItems = [
  {
    label: 'qallta ... tukuya',
    detail: 'Plantilla de programa',
    body: 'qallta\n\t$0\ntukuya'
  },
  {
    label: 'jisa',
    detail: 'Condicional if',
    body: 'jisa (${1:condicion}) {\n\t$0\n}'
  },
  {
    label: 'jisa ... maysatxa',
    detail: 'Condicional if/else',
    body: 'jisa (${1:condicion}) {\n\t$2\n} maysatxa {\n\t$0\n}'
  },
  {
    label: 'ukhakamaxa',
    detail: 'Bucle while',
    body: 'ukhakamaxa (${1:condicion}) {\n\t$0\n}'
  },
  {
    label: 'taki',
    detail: 'Bucle for',
    body: 'taki (${1:inicializacion}; ${2:condicion}; ${3:incremento}) {\n\t$0\n}'
  },
  {
    label: 'lurawi',
    detail: 'Definir función',
    body: 'lurawi ${1:nombre}(${2:parametros}) {\n\t$0\n}'
  },
  {
    label: 'kasta',
    detail: 'Declarar clase',
    body: 'kasta ${1:Nombre} {\n\t$0\n}'
  },
  {
    label: 'yant\'aña',
    detail: 'Bloque try/catch/finally',
    body: "yant'aña {\n\t$1\n} katjaña (${2:error}) {\n\t$3\n} tukuyawi {\n\t$0\n}"
  }
];

function toCompletionItems(items, kind) {
  return items.map((item) => {
    const completion = new vscode.CompletionItem(item.label, kind);
    completion.detail = item.detail;
    if (item.documentation) {
      completion.documentation = new vscode.MarkdownString(item.documentation);
    }
    return completion;
  });
}

function toSnippetItems(items) {
  return items.map((item) => {
    const completion = new vscode.CompletionItem(item.label, vscode.CompletionItemKind.Snippet);
    completion.detail = item.detail;
    completion.insertText = new vscode.SnippetString(item.body);
    return completion;
  });
}

function activate(context) {
  const provider = {
    provideCompletionItems() {
      return [
        ...toCompletionItems(keywordItems, vscode.CompletionItemKind.Keyword),
        ...toCompletionItems(typeItems, vscode.CompletionItemKind.TypeParameter),
        ...toCompletionItems(literalItems, vscode.CompletionItemKind.Value),
        ...toSnippetItems(snippetItems)
      ];
    }
  };

  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider({ language: 'aym' }, provider)
  );
}

function deactivate() {}

module.exports = {
  activate,
  deactivate
};
