#include "opengl.cpp"

static uint32_t areaID = 0;

static std::string const lorem_ipsum =
"Lorem ipsum dolor sit amet,\n"
"consectetur adipiscing elit.\n"
"Pellentesque vitae velit ante.\n"
"Suspendisse nulla lacus,\n"
"tempor sit amet iaculis non,\n"
"scelerisque\u001F1\u001F[ يرغب في الحب ]\u001F0\u001Fsed est.\n"
"Aenean pharetra ipsum sit amet sem lobortis,\n"
"a cursus felis semper.\n"
"Integer nec tortor ex.\n"
"Etiam quis consectetur turpis.\n"
"Proin ultrices bibendum imperdiet.\n"
"Suspendisse vitae fermentum ante,\n"
"eget cursus dolor.";

static std::string const lorem_ipsum2 =
"Lorem ipsum dolor sit amet, "
"consectetur adipiscing elit. "
"Nam ornare arcu turpis, "
"at suscipit purus rhoncus eu. "
"Donec vitae diam eget ante imperdiet mollis. "
"Maecenas id est sit amet sapien eleifend gravida. "
"Curabitur sit amet felis condimentum. "
"varius augue eu, "
"maximus nunc. "
"Curabitur quis laoreet tellus. "
"Donec urna arcu, "
"egestas vitae commodo euismod, "
"sodales vitae nunc.";

static std::string const arabic_lorem_ipsum =
"لكن لا بد أن أوضح لك أن كل هذه الأفكار المغلوطة حول\n"
"استنكار  النشوة وتمجيد الألم نشأت بالفعل،\n"
"وسأعرض لك التفاصيل لتكتشف حقيقة وأساس تلك السعادة البشرية،\n"
"فلا أحد يرفض أو يكره أو يتجنب الشعور بالسعادة،\n"
"ولكن بفضل هؤلاء الأشخاص الذين لا يدركون بأن السعادة لا بد أن نستشعرها\n"
"بصورة أكثر عقلانية ومنطقية فيعرضهم هذا لمواجهة الظروف الأليمة،\n"
"ونيل المنال ويتلذذ بالآلام،"
//"\u001F1\u001F يرغب في الحب \u001F0\u001F"
"\u001F1\u001F[ Lorem ipsum ]\u001F0\u001F"
"وأكرر بأنه لا يوجد من\n"
"الألم هو الألم ولكن نتيجة لظروف ما قد تكمن السعاده فيما نتحمله من كد .\n"
"و سأعرض مثال حي لهذا،\n"
"من منا لم يتحمل جهد بدني شاق إلا من أجل الحصول على ميزة أو\n"
"فائدة؟ ولكن من لديه الحق أن ينتقد شخص ما أراد أن يشعر بالسعادة التي لا\n"
"تشوبها عواقب أليمة أو آخر أراد أن يتجنب الألم الذي ربما تنجم عنه بعض ؟\n"
"علي الجانب الآخر نشجب ونستنكر هؤلاء الرجال المفتونون بنش";

int main() try
{
    using namespace SSS;

    //Log::TR::Fonts::get().glyph_load = true;

    TR::Format fmt;
    fmt.charsize = 30;
    fmt.has_outline = true;
    fmt.has_shadow = true;
    fmt.outline_size = 1;
    fmt.aligmnent = TR::Alignment::Center;
    fmt.effect = TR::Effect::Waves;
    fmt.effect_offset = -20;
    fmt.text_color.func = TR::ColorFunc::Rainbow;

    TR::Format fmt2 = fmt;
    fmt2.lng_tag = "ar";
    fmt2.lng_script = "Arab";
    fmt2.lng_direction = "rtl";
    fmt2.font = "LateefRegOT.ttf";
    TR::addFontDir("C:/dev/fonts");
    fmt2.aligmnent = TR::Alignment::Right;
    fmt.shadow_offset = { -3, 3 };
    fmt.effect_offset = 50;

    auto const& area = TR::Area::create(700, 700);
    area->setClearColor(0xFF888888);
    area->setFormat(fmt, 0);
    area->setFormat(fmt2, 1);
    area->parseString(lorem_ipsum);
    //area->setPrintMode(TR::Area::PrintMode::Typewriter);
    //area->setTypeWriterSpeed(60);
    areaID = area->getID();
    area->setFocus(true);

    // Create Window
    WindowPtr window;
    GLuint ids[5];
    glm::mat4 VP;
    load_opengl(window, ids, VP);

    // Main loop
    while (!glfwWindowShouldClose(window.get())) {

        // Poll events
        glfwPollEvents();
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // Bind needed things
        glUseProgram(ids[prog]);
        glBindVertexArray(ids[vao]);
        glBindTexture(GL_TEXTURE_2D, ids[tex]);
        glUniformMatrix4fv(glGetUniformLocation(ids[prog], "u_VP"), 1, false, &VP[0][0]);

        // Update texture if needed
        area->update();
        if (area->pixelsWereChanged()) {
            int w, h;
            area->pixelsGetDimensions(w, h);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, area->pixelsGet());
            area->pixelsAreRetrieved();
        }

        // Draw & print
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window.get());
    }

    glfwTerminate();
}
CATCH_AND_LOG_FUNC_EXC;