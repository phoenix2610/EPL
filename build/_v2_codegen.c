
/* ═══════════════════════════════════════════
   EPF v2.0 COMPONENT CODE GENERATORS
   ═══════════════════════════════════════════ */

/* ── Design system context (populated by NODE_USE_DESIGN) ── */
typedef struct {
    char primary[64];
    char secondary[64];
    char background[64];
    char headline_font[64];
    char body_font[64];
    char icon_set[64];
    int  has_material_icons;
} DesignCtx;

static DesignCtx g_design = {
    "#a3f69b", "#e9c349", "#131313",
    "Noto Serif", "Inter", "Material Symbols", 1
};

/* Inject Tailwind + Google Fonts + Material Icons head block */
static void gen_use_design(CodegenCtx *ctx, ASTNode *node) {
    if (strlen(node->as.use_design.primary)       > 0)
        strncpy(g_design.primary,       node->as.use_design.primary,       63);
    if (strlen(node->as.use_design.secondary)     > 0)
        strncpy(g_design.secondary,     node->as.use_design.secondary,     63);
    if (strlen(node->as.use_design.background)    > 0)
        strncpy(g_design.background,    node->as.use_design.background,    63);
    if (strlen(node->as.use_design.headline_font) > 0)
        strncpy(g_design.headline_font, node->as.use_design.headline_font, 63);
    if (strlen(node->as.use_design.body_font)     > 0)
        strncpy(g_design.body_font,     node->as.use_design.body_font,     63);

    /* Output CDN links + Tailwind config */
    fprintf(ctx->out,
        "<script src='https://cdn.tailwindcss.com?plugins=forms,container-queries'></script>\n"
        "<link href='https://fonts.googleapis.com/css2?family=%s:ital,wght@0,400;0,700;1,400"
        "&family=%s:wght@300;400;500;600&display=swap' rel='stylesheet'/>\n"
        "<link href='https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined"
        ":wght,FILL@100..700,0..1&display=swap' rel='stylesheet'/>\n"
        "<style>.material-symbols-outlined{font-variation-settings:'FILL' 0,'wght' 300,'GRAD' 0,'opsz' 24;}"
        ".glass-card{background:rgba(53,53,52,0.4);backdrop-filter:blur(24px);-webkit-backdrop-filter:blur(24px);}"
        ".text-glow{text-shadow:0 0 20px rgba(163,246,155,0.3);}"
        "#booking-modal{display:none;position:fixed;inset:0;z-index:100;background:rgba(0,0,0,0.8);"
        "backdrop-filter:blur(10px);align-items:center;justify-content:center;}"
        "#booking-modal.active{display:flex;animation:fadeIn 0.5s ease forwards;}"
        ".modal-content{background:%s;border:1px solid %s;padding:3rem;border-radius:1rem;"
        "text-align:center;color:#e5e2e1;transform:translateY(20px);animation:slideUp 0.5s ease forwards;}"
        "@keyframes slideUp{to{transform:translateY(0);}}"
        "@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}"
        ".animate-spin{animation:spin 1s linear infinite;display:inline-block;vertical-align:middle;}"
        "</style>\n"
        "<script>tailwind.config={darkMode:'class',theme:{extend:{colors:{"
        "'primary-fixed-dim':'%s','on-primary-fixed':'#002203','on-primary':'#003909',"
        "'primary-fixed':'%s','surface':'%s','background':'%s',"
        "'secondary':'%s','on-surface':'#e5e2e1',"
        "'on-surface-variant':'#bfcaba','surface-container-highest':'#353534',"
        "'surface-container-low':'#1c1b1b','surface-container':'#201f1f',"
        "'outline-variant':'#40493d','secondary-container':'#ae8d10',"
        "'on-secondary-container':'#342800'},"
        "fontFamily:{'headline':['%s'],'body':['%s'],'label':['%s']}}}}</script>\n",
        g_design.headline_font, g_design.body_font,
        g_design.background, g_design.primary,   /* modal bg/border */
        g_design.primary, g_design.primary,       /* primary-fixed-dim, primary-fixed */
        g_design.background, g_design.background, /* surface, background */
        g_design.secondary,                        /* secondary */
        g_design.headline_font, g_design.body_font, g_design.body_font
    );
}

/* ── NAVBAR ── */
static void gen_make_navbar(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div id='tailwind-wrapper' class='bg-surface text-on-surface font-body"
        " selection:bg-primary-fixed selection:text-on-primary-fixed'>\n"
        "<nav class='fixed top-0 w-full z-50 bg-surface/40 backdrop-blur-xl"
        " flex justify-between items-center px-12 py-8 max-w-[1920px] mx-auto'>\n"
        "<div class='font-headline text-2xl italic text-[#e5e2e1] ml-8'>%s</div>\n"
        "<div class='hidden md:flex items-center gap-12'>\n",
        node->as.make_navbar.brand
    );
    for (int i = 0; i < node->as.make_navbar.children.count; i++) {
        ASTNode *c = node->as.make_navbar.children.items[i];
        if (!c) continue;
        if (c->type == NODE_NAV_LINK) {
            fprintf(ctx->out,
                "<a class='font-body text-[0.6875rem] uppercase tracking-widest"
                " text-[#e5e2e1]/70 transition-all hover:bg-[#88d982]/10 p-2'"
                " href='%s'>%s</a>\n",
                c->as.nav_link.href, c->as.nav_link.label);
        } else if (c->type == NODE_ACTION_BTN) {
            fprintf(ctx->out,
                "<button class='interactive-btn bg-primary-fixed text-on-primary-fixed"
                " px-8 py-3 rounded-xl font-body text-[0.6875rem] uppercase tracking-widest"
                " hover:brightness-110 transition-all'"
                " data-modal='%s'>%s</button>\n",
                c->as.action_btn.modal_id, c->as.action_btn.label);
        }
    }
    fprintf(ctx->out, "</div>\n</nav>\n");
}

/* ── HERO ── */
static void gen_make_hero(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<header class='relative h-screen w-full flex items-center overflow-hidden'>\n"
        "<div class='absolute inset-0 z-0'>\n"
        "<img class='w-full h-full object-cover brightness-[0.%d]' src='%s'/>\n"
        "</div>\n"
        "<div class='relative z-10 w-full px-12 md:px-24 flex flex-col md:flex-row"
        " items-end justify-between'>\n"
        "<div class='mb-12 md:mb-0 max-w-3xl'>\n"
        "<h1 class='font-headline text-[3.5rem] md:text-[6rem] leading-none tracking-tight"
        " text-on-surface mb-6 opacity-90'>\n"
        "%s <br/><span class='italic text-primary-fixed-dim'>%s</span>\n"
        "</h1>\n"
        "<p class='font-label text-sm uppercase tracking-[0.3em] text-secondary opacity-80'>%s</p>\n"
        "</div>\n",
        node->as.make_hero.bg_dim,
        node->as.make_hero.bg_image,
        node->as.make_hero.title,
        node->as.make_hero.accented,
        node->as.make_hero.subtitle
    );
    if (node->as.make_hero.overlay_card) {
        ASTNode *oc = node->as.make_hero.overlay_card;
        fprintf(ctx->out,
            "<div class='glass-card p-12 rounded-xl max-w-md w-full"
            " border border-outline-variant/10 shadow-2xl md:translate-y-24'>\n"
            "<h3 class='font-headline text-2xl mb-8'>%s</h3>\n"
            "<div class='space-y-6'>\n",
            oc->as.overlay_card.title);
        for (int i = 0; i < oc->as.overlay_card.children.count; i++) {
            ASTNode *c = oc->as.overlay_card.children.items[i];
            if (!c) continue;
            if (c->type == NODE_GUEST_PICKER) {
                fprintf(ctx->out,
                    "<div class='group'><label class='font-label text-[0.6875rem]"
                    " uppercase tracking-widest text-secondary mb-2 block'>Guests</label>\n"
                    "<div class='bg-surface-container-highest/40 p-4 rounded-lg flex"
                    " items-center justify-between group-hover:bg-surface-container-highest/60"
                    " transition-colors'>\n"
                    "<span class='text-sm'>%d People</span>\n"
                    "<span class='material-symbols-outlined text-sm'>expand_more</span>\n"
                    "</div></div>\n", c->as.guest_picker.default_count);
            } else if (c->type == NODE_DATE_PICKER) {
                fprintf(ctx->out,
                    "<div class='group'><label class='font-label text-[0.6875rem]"
                    " uppercase tracking-widest text-secondary mb-2 block'>Date</label>\n"
                    "<div class='bg-surface-container-highest/40 p-4 rounded-lg flex"
                    " items-center justify-between group-hover:bg-surface-container-highest/60"
                    " transition-colors'>\n"
                    "<span class='text-sm'>%s</span>\n"
                    "<span class='material-symbols-outlined text-sm'>calendar_today</span>\n"
                    "</div></div>\n", c->as.date_picker.label);
            } else if (c->type == NODE_ACTION_BTN) {
                fprintf(ctx->out,
                    "<button class='interactive-btn w-full bg-primary-fixed"
                    " text-on-primary-fixed py-5 rounded-xl font-label text-[0.6875rem]"
                    " uppercase tracking-[0.2em] font-bold hover:brightness-110"
                    " transition-all shadow-lg' data-modal='%s'>%s</button>\n",
                    c->as.action_btn.modal_id, c->as.action_btn.label);
            }
        }
        fprintf(ctx->out, "</div></div>\n");
    }
    fprintf(ctx->out, "</div>\n</header>\n");
}

/* ── CONTENT SECTION ── */
static void gen_content_section(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section id='%s' class='py-48 px-12 md:px-24 max-w-[1920px] mx-auto overflow-hidden'>\n"
        "<div class='grid grid-cols-1 md:grid-cols-12 gap-24 items-start'>\n",
        node->as.content_section.id);

    /* Left column */
    if (node->as.content_section.left_col) {
        fprintf(ctx->out, "<div class='md:col-span-5 space-y-24'>\n");
        ASTNode *col = node->as.content_section.left_col;
        for (int i = 0; i < col->as.column.children.count; i++)
            codegen_node(ctx, col->as.column.children.items[i]);
        fprintf(ctx->out, "</div>\n");
    }

    /* Right column */
    if (node->as.content_section.right_col) {
        fprintf(ctx->out, "<div class='md:col-span-7 relative'>\n");
        ASTNode *col = node->as.content_section.right_col;
        for (int i = 0; i < col->as.column.children.count; i++)
            codegen_node(ctx, col->as.column.children.items[i]);
        fprintf(ctx->out, "</div>\n");
    }
    fprintf(ctx->out, "</div>\n</section>\n");
}

/* ── EYEBROW ── */
static void gen_eyebrow(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<span class='font-label text-[0.6875rem] uppercase tracking-[0.4em]"
        " text-primary-fixed-dim mb-4 block'>%s</span>\n",
        node->as.eyebrow.text);
}

/* ── ITEM LIST ── */
static void gen_item_list(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out, "<div class='space-y-16 pl-8 border-l border-outline-variant/20'>\n");
    for (int i = 0; i < node->as.item_list.entries.count; i++) {
        ASTNode *e = node->as.item_list.entries.items[i];
        if (!e || e->type != NODE_LIST_ENTRY) continue;
        const char *offset = e->as.list_entry.offset ? " md:translate-x-12" : "";
        fprintf(ctx->out,
            "<div class='group cursor-default%s'>\n"
            "<div class='flex justify-between items-baseline mb-2'>\n"
            "<h4 class='font-headline text-xl group-hover:text-primary-fixed transition-colors'>%s</h4>\n"
            "<span class='font-headline text-4xl text-secondary'>%02d</span></div>\n"
            "<p class='text-sm text-on-surface-variant italic'>%s</p>\n"
            "</div>\n",
            offset, e->as.list_entry.label,
            e->as.list_entry.number, e->as.list_entry.detail);
    }
    fprintf(ctx->out, "</div>\n");
}

/* ── FEATURE IMAGE ── */
static void gen_feature_image(CodegenCtx *ctx, ASTNode *node) {
    const char *aspect = strlen(node->as.feature_image.aspect) > 0
                         ? node->as.feature_image.aspect : "4/5";
    int reveal = strstr(node->as.feature_image.hover_mode, "color") != NULL;
    fprintf(ctx->out,
        "<div class='aspect-[%s] bg-surface-container-low overflow-hidden rounded-xl'>\n"
        "<img class='w-full h-full object-cover%s transition-all duration-1000' src='%s'/>\n"
        "</div>\n",
        aspect,
        reveal ? " grayscale hover:grayscale-0" : "",
        node->as.feature_image.src);
}

/* ── PRICE BADGE ── */
static void gen_price_badge(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div class='absolute -bottom-12 -left-12 glass-card p-12 rounded-xl hidden md:block'>\n"
        "<span class='font-label text-[0.6875rem] uppercase tracking-widest"
        " text-secondary block mb-2'>The Experience</span>\n"
        "<span class='font-headline text-5xl'>%s</span>\n"
        "<span class='font-label text-xs uppercase ml-2 text-on-surface-variant'>%s</span>\n"
        "</div>\n",
        node->as.price_badge.price, node->as.price_badge.label);
}

/* ── GALLERY ── */
static void gen_make_gallery(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section class='py-48 bg-surface-container-low/30'>\n"
        "<div class='px-12 md:px-24 mb-24 text-center'>\n"
        "<h2 class='font-headline text-[4rem] mb-6'>%s</h2>\n"
        "<p class='font-label text-xs uppercase tracking-[0.5em] text-on-surface-variant'>%s</p>\n"
        "</div>\n"
        "<div class='grid grid-cols-1 md:grid-cols-%d gap-4 px-4 h-[800px]'>\n",
        node->as.make_gallery.title,
        node->as.make_gallery.subtitle,
        node->as.make_gallery.columns);

    for (int i = 0; i < node->as.make_gallery.images.count; i++) {
        ASTNode *img = node->as.make_gallery.images.items[i];
        if (!img || img->type != NODE_GALLERY_IMAGE) continue;
        const char *mt = img->as.gallery_image.offset_top > 0 ? " md:mt-24" : "";
        fprintf(ctx->out,
            "<div class='relative overflow-hidden group h-full%s'>\n"
            "<img class='w-full h-full object-cover grayscale brightness-75"
            " group-hover:scale-110 transition-transform duration-1000' src='%s'/>\n"
            "<div class='absolute inset-0 bg-gradient-to-t from-surface"
            " to-transparent opacity-60'></div>\n"
            "</div>\n",
            mt, img->as.gallery_image.src);
    }
    fprintf(ctx->out, "</div>\n</section>\n");
}

/* ── FORM ── */
static void gen_make_form(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<section class='py-48 px-12 md:px-24 flex flex-col items-center"
        " justify-center text-center'>\n"
        "<div class='max-w-2xl w-full'>\n"
        "<span class='font-label text-[0.6875rem] uppercase tracking-[0.5em]"
        " text-secondary mb-6 block'>%s</span>\n"
        "<h2 class='font-headline text-[3.5rem] mb-12'>%s</h2>\n"
        "<div class='space-y-8 text-left'>\n"
        "<div class='grid grid-cols-1 md:grid-cols-2 gap-8'>\n",
        node->as.make_form.eyebrow, node->as.make_form.title);

    int in_grid = 1;
    for (int i = 0; i < node->as.make_form.children.count; i++) {
        ASTNode *c = node->as.make_form.children.items[i];
        if (!c) continue;
        if (c->type == NODE_FORM_FIELD) {
            int is_textarea = strcmp(c->as.form_field.field_type, "textarea") == 0;
            if (is_textarea && in_grid) {
                fprintf(ctx->out, "</div>\n"); in_grid = 0;
            }
            if (is_textarea) {
                fprintf(ctx->out,
                    "<div class='relative'>\n"
                    "<label class='font-label text-xs text-on-surface-variant"
                    " uppercase tracking-widest mb-3 block'>%s</label>\n"
                    "<textarea class='w-full bg-surface-container-highest border-none p-5"
                    " rounded-lg focus:ring-1 focus:ring-primary-fixed/30 transition-all"
                    " placeholder:text-on-surface-variant/30 text-sm h-32'"
                    " placeholder='%s'></textarea>\n"
                    "</div>\n",
                    c->as.form_field.field_label, c->as.form_field.placeholder);
            } else {
                fprintf(ctx->out,
                    "<div class='relative'>\n"
                    "<label class='font-label text-xs text-on-surface-variant"
                    " uppercase tracking-widest mb-3 block'>%s</label>\n"
                    "<input class='w-full bg-surface-container-highest border-none p-5"
                    " rounded-lg focus:ring-1 focus:ring-primary-fixed/30 transition-all"
                    " placeholder:text-on-surface-variant/30 text-sm'"
                    " placeholder='%s' type='%s'/>\n"
                    "</div>\n",
                    c->as.form_field.field_label,
                    c->as.form_field.placeholder,
                    c->as.form_field.field_type);
            }
        } else if (c->type == NODE_SUBMIT_BTN) {
            fprintf(ctx->out,
                "<button class='interactive-btn w-full bg-secondary-container"
                " text-on-secondary-container py-6 rounded-xl font-label text-[0.75rem]"
                " uppercase tracking-[0.3em] font-bold hover:brightness-110"
                " transition-all' data-modal='%s'>%s</button>\n",
                c->as.submit_btn.modal_id, c->as.submit_btn.label);
        }
    }
    fprintf(ctx->out, "</div></div></section>\n");
}

/* ── MODAL ── */
static void gen_make_modal(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<div id='%s'>\n"
        "<div class='modal-content'>\n"
        "<span class='material-symbols-outlined'"
        " style='font-size:4rem;color:%s;margin-bottom:1rem;'>%s</span>\n"
        "<h2 class='font-headline text-3xl mb-4'>%s</h2>\n"
        "<p class='font-body text-on-surface-variant mb-8'>%s</p>\n"
        "<button id='close-%s' class='bg-surface-container-highest px-8 py-3"
        " rounded-xl font-label text-sm hover:bg-surface-variant"
        " transition-colors'>%s</button>\n"
        "</div></div>\n"
        "<script>\n"
        "document.querySelectorAll('[data-modal=\"%s\"]').forEach(btn=>{\n"
        "  btn.addEventListener('click',function(e){\n"
        "    e.preventDefault();\n"
        "    const orig=this.innerHTML;\n"
        "    this.innerHTML='<span class=\"material-symbols-outlined animate-spin\""
        " style=\"font-size:1.2rem;margin-right:0.5rem;\">sync</span> Processing...';\n"
        "    this.style.opacity='0.8';this.style.pointerEvents='none';\n"
        "    setTimeout(()=>{\n"
        "      this.innerHTML=orig;this.style.opacity='1';this.style.pointerEvents='auto';\n"
        "      document.getElementById('%s').classList.add('active');\n"
        "    },1500);\n"
        "  });\n"
        "});\n"
        "document.getElementById('close-%s').addEventListener('click',()=>{\n"
        "  document.getElementById('%s').classList.remove('active');\n"
        "});\n"
        "</script>\n",
        node->as.make_modal.modal_id,
        strcmp(node->as.make_modal.color, "primary") == 0 ? g_design.primary : g_design.secondary,
        node->as.make_modal.icon,
        node->as.make_modal.heading,
        node->as.make_modal.body_text,
        node->as.make_modal.modal_id,
        node->as.make_modal.dismiss_label,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id,
        node->as.make_modal.modal_id);
}

/* ── FOOTER ── */
static void gen_make_footer(CodegenCtx *ctx, ASTNode *node) {
    fprintf(ctx->out,
        "<footer class='bg-[%s] w-full pt-24 pb-12 flex flex-col md:flex-row"
        " justify-between items-end px-20 gap-24'>\n"
        "<div class='flex-1'>\n"
        "<div class='font-headline text-4xl text-[#e5e2e1] mb-12'>%s</div>\n"
        "<p class='font-body text-[0.875rem] leading-relaxed text-[#e5e2e1]/50 max-w-sm'>%s</p>\n"
        "</div>\n"
        "<div class='text-right'>\n"
        "<p class='font-headline text-[2rem] text-[%s] mb-4'>%s</p>\n"
        "<p class='font-body text-[0.875rem] leading-relaxed text-[#e5e2e1]/50'>%s</p>\n"
        "<div class='mt-12 text-[#e5e2e1]/30 font-body text-[0.6875rem]"
        " uppercase tracking-widest'>© %s</div>\n"
        "</div>\n"
        "</footer>\n"
        "</div>\n", /* close tailwind-wrapper */
        g_design.background,
        node->as.make_footer.brand,
        node->as.make_footer.tagline,
        g_design.secondary,
        node->as.make_footer.contact_heading,
        node->as.make_footer.address,
        node->as.make_footer.copyright);
}
