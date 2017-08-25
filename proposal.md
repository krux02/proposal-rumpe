# Sehr geehrter Herr Rumpe,

schon seit längerem arbeitet ihr Lehrstuhl an Monticore, einem Tool
für die Entwicklung von _embedded DSLs_. Ich habe schon seit längerem
ein Konzept für eine solche DSL.  Meine Idee wäre es diese DSL mit
ihrer Kooperation mit Monticore als Masterarbeit an ihrem Lehrstuhl
zu entwickeln.

# Worum geht es:

Es geht darum, die Entwicklung von Visualisierungen und allgemeinen
Berechnungen zu beschleunigen. Meiner Erfahrung nach geht immer dann
wenn ein komplett neuer Renderer für ein beliebiges Problem
geschrieben werden muss, sehr viel Arbeitszeit darin verloren, die
Daten aus dem Arbeitsspeicher, welche zur Visualisierung genutzt
werden sollen, im Programm auf der GPU verfügbar zu machen. Das
Symptom welches daraus resultiert ist, dass oft allgemeine Renderer
geschrieben werden, die alles können wollen, und beliebig
konfigurierbar sind, anstelle von einfachen Renderern, die nur das
machen und die Dateien verarbeiten, für die sie geschrieben
wurden. Auswirkungen hat dies dann in der Performance, wenn des
gewünschte Ergebnis nur wenig Anspruch auf Photorealität hat.

Einfach um mal ein Beispiel zu nennen, bei dem ich stark vermute dass
dieser Zusammenhang hier existiert. Das Spiel Broforce hat triviale
Grafik, allerdings keine trivialen Ansprüche an die
Grafikleistung. Dies kann natürlich auch andere Gründe haben. Aber mit
der DSL die ich hier vorstelle, wäre es sehr einfach sehr effizienten
Renderer für solche Daten zu schreiben.

Um die DSL verstehen zu können gebe ich hier noch mal eine kleine
vereinfachte Zusammenfassung, wie die Rendering-Pipeline funktioniert.

Als aller erstes braucht die Grafikkarte eine Punktwolke aus Daten,
welche die Vertices der 3D Geometrie (Mesh) repräsentieren. Jeweils drei
Vertices zusammen ergeben ein Dreieck auf der Oberfläche des
Mesh. Vertices können beliebige zusätzliche Daten zu ihrer Position
haben, und auch wie die Position gespeichert wird ist Sache des
Programmierers, es gibt kein festes Layout dafür.

Als nächstes werden die Vertices im Vertexshader
verarbeitet. Der Vertexshader ist hier ein Rechenkernel, der auf
jedem Vertex ausgeführt wird. Aufgabe des Vertexshaders ist es, die
Vertexpositionen entsprechend der Kameraposition und der der
perspektivischen Verzerrung zu transformieren. Bei OpenGL müssen die
Positionen im Bereich [-1; 1] liegen, um sichtbar zu sein. Die
original Daten bleiben hier unverändert im Speicher, der Vertexshader
hat hier reinen Lesezugriff. Der Vertexshader bestimmt auch, wie die
anderen Attribute, wie zum Beispiel Texturkoordinaten verarbeitet
werden.

Die _Rasterization-Stage_ ist ein nicht programmierbarer teil der
Rendering-Pipeline. Bis hier sind alle Daten nur pro Vertex
definiert.  Dreiecke haben also Daten an allen Ecken definiert, und der
Rasterizer hat die Aufgabe, diese Daten über das Dreieck
perspektivisch korrekt zu interpolieren, so dass ein Wert pro
Pixel existiert.

Der nächste Schritt in der Pipeline ist der Fragmentshader (bei
DirectX auch Pixelshader genannt).  Dieser berechnet aus seinen
Eingaben nun letztendlich die Farbe für die Darstellung des
Pixels. Auch dies ist ein programmierbarer Kernel mit beliebig vielen
eingaben. Dieser Shader übernimmt zum Beispiel die Beleuchtung und
Texturierung. Besonders für die Texturierung ist die Renderingpipeline
sehr hilfreich weil es viele Schritte für Mipmapping automatisch
übernimmt. Eine einfache schleife in c++ könnte diesen Teil deshalb
nicht so einfach ersetzen.

Die Kernidee meiner DSL ist es den shader code (GLSL), dort in den C++
code zu integrieren, wo die Daten auf die den Shader zugreifen muss, im
lokalen Scope sind.

Im konkreten Beispiel sieht das wie folgt aus:

Lokale oder globale Variablen in C++ als sicht auf die Daten:

    ArrayBuffer<glm::vec4> positions;
    ArrayBuffer<glm::vec4> colors;
    glm::mat4 modelViewMat;
    glm::mat4 projMat;

`ArrayBuffer` ist hier ein Typ vergleichbar mit `std::vector` mit
zusammenhängendem Speicher für alle Elemente. Der Hauptunterschied zu
`std::vector` ist allerdings, dass der Speicher auf der Grafikkarte
liegt. Ein ArrayBuffer speichert pro vertex Attribute. `modelViewMat`
und `projMat` sind Variablen, die zwar veränderbar sind aber pro
Aufruf des Shaderprogrammes nur noch den selben wert für sowohl
Vertex-, als auch Fragmentshader haben, und das über alle Vertices and
Pixel hinweg.  Diese Daten heißen bei OpenGL `uniform`.

So könnte die Anbindung der Daten an den GLSL code aussehen:

    // this here is normal c++ code

    SHADING_DSL(R"dsl(
      uniforms {
        modelView = modelViewMat;
        proj = projMat;
      }
      attributes{
        a_vertex = vertices;
        a_color = colors;
      }
      vertexMain{
        gl_Position = proj * modelView * a_vertex;
        v_color = a_color;
      }
      vertexOut{
        out vec4 v_color;
      }
      fragmentMain{
        color = v_color;
      }
    )dsl");

Für c++ ist die DSL einfach nur ein mehrzeiliges Stringliteral. Im
Block `uniforms` und `attributes`, werden die die Namen der c++
Variables benutzt um die _uniforms_ und _attributes_ für den shader
zu definieren. Also von dem Datentyp von `modelViewMat` (`glm::mat4`)
leitet sich das Uniform `"uniform mat4 modelView"` ab. Von der Variablen
`vertices` (`ArrayBuffer<glm::vec4>`) leitet sich das Attribut
`"attribute vec4 a_vertex"` ab. In `vertexMain` und `fragmentMain`
befindet sich GLSL code. Sowohl der code in `vertexMain` als auch in
`fragmentMain` soll alle Variable die in `uniforms` definiert worden
sind nutzen können. `vertexMain` soll zusätzlich noch die Variablen aus
`attributes` nutzen können und in die variables aus `vertexOut`
schreiben können und `fragmentMain` soll aus den Daten aus `vertexOut`
lesen können.

uniforms:
  Each uniform to the shader is represented here as an
  assignment. The identifier on the left side of each assignment
  should define how the uniform can be used from the shader. The
  expression on the right of the assignment should be a c++ symbol
  that sets the value and the type of the uniform. There has to be
  done some type mapping. In this case modelViewMat is of type
  glm::mat4, in GLSL modelView should be of type mat4, the GLSL
  mapping of that type.

attributes:
  Each attribute to the shader is represented here as an
  assignment. The identifier on the left side of each assignment
  should define how the attribute can be used from the shader. The
  expression on the right of the assignment should be a c++ symbol
  that sets the value and the type of the attribute. There should be
  done some type mapping. In this case vertices is of type
  ArrayBuffer<glm::vec4>, in GLSL a_vertex should be of type vec4, the
  GLSL attribute mapping of that type.

vertexMain:
  This is the code that will be inserted in the main function of the
  vertex shader. As you can see, shader code does not need to be
  complicated, when there is no complicated shading happening.

vertexOut:
  This is simply the GLSL code that would be written in the shader, to
  declare the output types of the vertex shader. The only reason to
  put it in a special block, is to reuse that code block in the
  fragment shader as inputs.

fragmentMain:
  This is the code for the fragment shader. In this case it simply
  forwards the input color from the rasterization stage as output
  color. Inputs are generated from the "vertexOut" section, but with
  "out" replaced by "in".

    /** uniforms:
      The uniforms section should be a list of assignments. Each identifier on the left hand side of an assignment should be the name, how the uniform is used in GLSL*/
