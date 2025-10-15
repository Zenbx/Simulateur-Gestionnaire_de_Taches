#include <gtk/gtk.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <math.h>



sem_t resource_semaphore;
pthread_mutex_t resource_mutex;
GtkWidget *grid;
GtkWidget *grid2;
GtkWidget *progress_grid;
GtkWidget *chart_area;
int prime_current_row = 0;
int current_row = 3;
int current_row2 = 2;
int current_progress_row = 0;
double max_resources = 100.0;  // Maximum de ressources disponibles
double used_resources = 0.0;   // Ressources actuellement utilisées
time_t start;
gboolean should_stop_update = FALSE;
GtkWidget *window ;
 GtkWidget *custom_box;

typedef struct{
    gchar *task_name;
    GtkWidget *status_label;
    int start_delay;
    gboolean is_complete;
    gboolean in_progress;
    gboolean is_scheluded;
    int nbr_processus;


} Task;

typedef struct {
    gchar *task_name;
    GtkWidget *status_label;
    double resource_percentage;
    int memory_time;
    int start_delay;
    GtkWidget *progress_bar;
    GtkWidget *drawing_area;
    GtkWidget *message_view;
    GtkWidget *launch_button;
    GtkTextBuffer *message_buffer;
    double fraction;
    gboolean is_complete;
    gboolean in_progress;
    gboolean is_scheluded;
    time_t start_time;
    int id_priority;
    gboolean use_mutex;
} ThreadData;// Structure d'une tâche

// Structure pour stocker les données nécessaires pour l'animation
typedef struct {
    GtkWidget *widget;
    int y_start;
    int y_end;
    int step;
    double opacity;
} AnimationData;



GList *task_list = NULL;// Liste des processus
GList *tache_liste = NULL; // LIste de tâches
GList *task_list2 = NULL; // Liste de processus de taches

// Ajouter une tâche et initialiser ses données
ThreadData *add_task(GtkWidget *button, gpointer data);
static gboolean auto_start_task(gpointer data);
static gboolean auto_start_tache(gpointer data);
static void manual_start_task(GtkWidget *button, gpointer data);
void init_circular_progress_bar(ThreadData *thread_data, GtkWidget *parent_box);
static void append_task_message(ThreadData *thread_data, const gchar *message);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static void* simulate_thread_processing(void *data);


// Fonction de démarrage automatique basé sur le start_delay
static gboolean auto_start_task2(gpointer data) {
    time_t current_time = time(NULL);
    GList *l;

    for (l = task_list2; l != NULL; l = l->next) {
        ThreadData *thread_data = (ThreadData *)l->data;
        if (!thread_data->is_complete && !thread_data->in_progress &&
            (used_resources + thread_data->resource_percentage) <= max_resources &&
            (current_time >= thread_data->start_delay)) {
            used_resources += thread_data->resource_percentage;
            thread_data->in_progress = TRUE;


            pthread_t thread;
            if (pthread_create(&thread, NULL, simulate_thread_processing, thread_data) == 0) {
                pthread_detach(thread);
            } else {
                g_warning("Erreur lors de la création du thread pour la tâche : %s", thread_data->task_name);
                used_resources -= thread_data->resource_percentage;
                thread_data->in_progress = FALSE;
            }
        }
    }
    return G_SOURCE_CONTINUE;

}

// Fonction de démarrage automatique basé sur le start_delay
static gboolean auto_start_tache(gpointer data) {
    time_t current_time = time(NULL);
    GList *l;
    int j = 0;
    srand(time(NULL));
    for (l = tache_liste; l != NULL; l = l->next) {
        Task *task = (Task *)l->data;
        gtk_label_set_text(GTK_LABEL(task->status_label), "État de la tâche: En cours");
        //current_row2 = prime_current_row;
        if (!task->is_complete && !task->in_progress ){
            task->in_progress = TRUE;

            for(int i = 1 ; i<= task->nbr_processus; i++){
                printf("Ajout du Processus %d",i);
                //Ajout des attributs de la nouvelle tâche
                ThreadData *thread_data = g_malloc(sizeof(ThreadData));
                thread_data->task_name = g_strdup_printf("Processus %d de la tâche %s",i,task->task_name);
                thread_data->resource_percentage = (rand() % 100) + 1;
                thread_data->start_delay = (rand() % 10) + 1;
                thread_data->id_priority = (rand() % 3) + 1;
                thread_data->is_complete = FALSE;
                thread_data->in_progress = FALSE;
                thread_data->memory_time =(rand() % 10)+1;
                thread_data->is_scheluded = thread_data->start_delay >= 0;
                //thread_data->use_mutex = TRUE;
                // Crée la zone de dessin pour la barre circulaire
                GtkWidget *drawing_area = gtk_drawing_area_new();
                gtk_widget_set_size_request(drawing_area, 50, 50);

                g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), thread_data);


                const gchar *memory_time_text = g_strdup_printf("%d", thread_data->memory_time);
                const gchar *resource_percentage_text = g_strdup_printf("%f", thread_data->resource_percentage);
                const gchar *id_priority_text = g_strdup_printf("%d", thread_data->id_priority);

                // Création d'une cellule de nom de tâche avec bordure
                GtkWidget *label1 = gtk_label_new(thread_data->task_name);
                gtk_style_context_add_class(gtk_widget_get_style_context(label1), "grid-cells");

                // Création d'une cellule de statut avec bordure
                thread_data->status_label = gtk_label_new("Pas fait");
                gtk_style_context_add_class(gtk_widget_get_style_context(thread_data->status_label), "grid-cell");

                // Création d'une cellule de pourcentage de ressources avec bordure
                GtkWidget *label3 = gtk_label_new(resource_percentage_text);
                gtk_style_context_add_class(gtk_widget_get_style_context(label3), "grid-cell");

                // Création d'une cellule de temps mémoire avec bordure
                GtkWidget *label4 = gtk_label_new(memory_time_text);
                gtk_style_context_add_class(gtk_widget_get_style_context(label4), "grid-cell");

                GtkWidget *label5 = gtk_label_new(id_priority_text);
                gtk_style_context_add_class(gtk_widget_get_style_context(label5), "grid-cell");

                // Ajout des widgets dans la grille
                gtk_grid_attach(GTK_GRID(grid2), label1, 0, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), thread_data->status_label, 1, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label3, 2, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label4, 3, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label5, 4, current_row2, 1, 1);


                //thread_data->progress_bar = gtk_progress_bar_new();
                //gtk_widget_set_name(thread_data->progress_bar,"progress-bar");

               GtkWidget *parent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

               init_circular_progress_bar(thread_data,parent_box);


                GtkWidget *task_label = gtk_label_new(thread_data->task_name);
                //gtk_grid_attach(GTK_GRID(progress_grid), task_label, 0, current_progress_row, 1, 1);
                //gtk_grid_attach(GTK_GRID(progress_grid), drawing_area, 0, current_progress_row + 1, 1, 1);

                gtk_grid_attach(GTK_GRID(progress_grid), parent_box, 0, current_progress_row , 1, 1);


                if(thread_data->start_delay>15){
                    thread_data->launch_button = gtk_button_new_with_label("Lancer");
                    gtk_widget_set_name(thread_data->launch_button, "button-3D");
                    g_signal_connect(thread_data->launch_button, "clicked" , G_CALLBACK(manual_start_task), thread_data);
                    gtk_grid_attach(GTK_GRID(grid), thread_data->launch_button, 5, current_row, 1, 1);
                }

                g_timeout_add(thread_data->start_delay, (GSourceFunc)auto_start_task2, thread_data);


                thread_data->message_view = gtk_text_view_new();
                gtk_widget_set_name(thread_data->message_view,"table-view");
                gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(thread_data->message_view), GTK_WRAP_WORD_CHAR);
                gtk_text_view_set_editable(GTK_TEXT_VIEW(thread_data->message_view), FALSE);
                thread_data->message_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(thread_data->message_view));
                gtk_widget_set_size_request(thread_data->message_view, 250, 20);

                gtk_grid_attach(GTK_GRID(progress_grid), thread_data->message_view, 1, current_progress_row , 1, 1);

                //Partir à la ligne suivante
                current_row2++;
                current_progress_row += 1;

                gtk_widget_show_all(grid2);
                gtk_widget_show_all(progress_grid);


                //Ajout des élements à la liste
                task_list2 = g_list_append(task_list2, thread_data);
                j++ ;
            }

            }
             // Création d'une cellule de nom de tâche avec bordure
                GtkWidget *label6 = gtk_label_new(" ");
                gtk_style_context_add_class(gtk_widget_get_style_context(label6), "grid-cells");

                // Création d'une cellule de statut avec bordure
                GtkWidget *label7 = gtk_label_new(" ");
                gtk_style_context_add_class(gtk_widget_get_style_context(label7), "grid-cell");

                // Création d'une cellule de pourcentage de ressources avec bordure
                GtkWidget *label8 = gtk_label_new(" ");
                gtk_style_context_add_class(gtk_widget_get_style_context(label8), "grid-cell");

                // Création d'une cellule de temps mémoire avec bordure
                GtkWidget *label9 = gtk_label_new(" ");
                gtk_style_context_add_class(gtk_widget_get_style_context(label9), "grid-cell");

                GtkWidget *label10 = gtk_label_new(" ");
                gtk_style_context_add_class(gtk_widget_get_style_context(label10), "grid-cell");

                // Ajout des widgets dans la grille
                gtk_grid_attach(GTK_GRID(grid2), label6, 0, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label7, 1, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label8, 2, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label9, 3, current_row2, 1, 1);
                gtk_grid_attach(GTK_GRID(grid2), label10, 4, current_row2, 1, 1);

            current_row2 += 3;
            if(!task->is_complete && j == task->nbr_processus){
                task->is_complete = TRUE;
                gtk_label_set_text(GTK_LABEL(task->status_label), "État de la tâche: Fait");
            }
    }

    return G_SOURCE_CONTINUE;
}

void add_tache(GtkWidget *button,gpointer data){
    GtkWidget **entries = (GtkWidget **)data;
    const gchar *task_name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const gchar *start_delay_text = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    int start_delay = g_ascii_strtod(start_delay_text,NULL);


    //Ajout des attributs de la nouvelle tâche
    Task *task= g_malloc(sizeof(Task));
    task->task_name = g_strdup(task_name);
    task->start_delay = start_delay;
    task->is_complete = FALSE;
    task->in_progress = FALSE;
    task->is_scheluded = start_delay >= 0;
    task->nbr_processus = (rand() % 10) + 1;

    // Création d'une cellule de nom de tâche avec bordure
    GtkWidget *label = gtk_label_new(task->task_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "grid-cells");

    // Création d'une cellule de statut avec bordure
    task->status_label = gtk_label_new("Ètat de la tâche: Pas fait");
    gtk_style_context_add_class(gtk_widget_get_style_context(task->status_label), "grid-cells");

    // Ajout des widgets dans la grille
    gtk_grid_attach(GTK_GRID(grid2), label, 0, prime_current_row, 3, 1);
    gtk_grid_attach(GTK_GRID(grid2), task->status_label, 3, prime_current_row, 2, 1);

     GtkWidget *title_name2 = gtk_label_new("Nom du processus");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_name2), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid2), title_name2, 0, prime_current_row + 1, 1, 1);

    GtkWidget *title_status_for_process2 = gtk_label_new("État");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_status_for_process2), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid2), title_status_for_process2, 1, prime_current_row + 1, 1, 1);

    GtkWidget *title_memory2 = gtk_label_new("Mémoire(%)");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_memory2), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid2), title_memory2, 2, prime_current_row + 1, 1, 1);

    GtkWidget *title_time2 = gtk_label_new("Temps Mémoire");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_time2), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid2), title_time2, 3, prime_current_row + 1, 1, 1);

    GtkWidget *title_priority2 = gtk_label_new("Priorité de la tâche(1, 2 ou 3)");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_priority2), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid2), title_priority2, 4, prime_current_row + 1, 1, 1);


    //g_timeout_add(task->start_delay, (GSourceFunc)auto_start_tache, task);
    prime_current_row += task->nbr_processus + 2;
     gtk_widget_show_all(grid2);

    //Effacer le contenu après ajout des tâches
    gtk_entry_set_text(GTK_ENTRY(entries[0]), "");
    gtk_entry_set_text(GTK_ENTRY(entries[1]), "");

    //Ajout des élements à la liste
    tache_liste = g_list_append(tache_liste, task);

}


// Fonction pour initialiser le temps de référence
void initialize_start_time() {
    start = time(NULL); // Initialiser le temps de référence au moment de l'exécution
}
// Fonction de dessin pour le diagramme
static gboolean on_draw_chart(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width, height;
    gtk_widget_get_size_request(widget, &width, &height);

    // Fond blanc
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Échelles
    cairo_set_line_width(cr, 2);
    cairo_set_source_rgb(cr, 0, 0, 0);

    cairo_move_to(cr, 50, height - 20);
    cairo_line_to(cr, width - 20, height - 20); // Axe du temps


    cairo_move_to(cr, 50, height - 20);
    cairo_line_to(cr, 50, 30); // Axe des ressources


    cairo_stroke(cr);

    // Label des axes
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 30, 20);
    cairo_show_text(cr, "Ressources (%)");
    cairo_move_to(cr, width - 10, height - 10);
    cairo_show_text(cr, "Temps");

     // Graduations sur l'axe des ressources
    int num_graduations = 5;
    for (int i = 0; i <= num_graduations; i++) {
        double y = height - 30 - (i * (height - 60) / num_graduations);
        double resource_value = (max_resources / num_graduations) * i;

        cairo_move_to(cr, 50, y);
        cairo_line_to(cr, 45, y);  // Petite ligne de graduation
        cairo_stroke(cr);

        // Texte de la graduation
        gchar *text = g_strdup_printf("%.0f", resource_value);
        cairo_move_to(cr, 20, y + 5);
        cairo_show_text(cr, text);
        g_free(text);
    }

    // Graduations sur l'axe du temps
    int time_interval = 5;  // Intervalle de temps arbitraire pour la démonstration
    for (int i = 0; i <= num_graduations; i++) {
        double x = 50 + i * (width - 70) / num_graduations;
        double time_value = i * time_interval;

        cairo_move_to(cr, x, height - 30);
        cairo_line_to(cr, x, height - 25);  // Petite ligne de graduation
        cairo_stroke(cr);

        // Texte de la graduation
        gchar *text = g_strdup_printf("%d", (int)time_value);
        cairo_move_to(cr, x - 5, height - 10);
        cairo_show_text(cr, text);
        g_free(text);
    }

// Tracer les lignes représentant les ressources au cours du temps
    cairo_set_line_width(cr, 1.5);
    for (GList *l = task_list; l != NULL; l = l->next) {
        ThreadData *task = l->data;
        if (task->is_complete) {
            double start_time_offset = difftime(task->start_time, ((ThreadData *)task_list->data)->start_time);
            int task_duration = task->memory_time * 100; // Ex : chaque unité de `memory_time` = 100ms

            // Points de départ pour chaque tâche sur le graphique
            int x_start = 50 + (int)(start_time_offset * 5);
            int y_start = height - 30 - (task->resource_percentage / max_resources * (height - 60));

            // Couleur aléatoire pour chaque ligne
            cairo_set_source_rgb(cr, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX);

            // Dessin d'une ligne point par point pour la tâche
            cairo_move_to(cr, x_start, y_start);
            for (int x = x_start + 1; x <= x_start + task_duration; x++) {
                double y = y_start;
                cairo_line_to(cr, x, y);
            }
            cairo_stroke(cr);
        }
    }

    return FALSE;

}

// Fonction pour dessiner un cadre lumineux avec un texte lumineux au centre
static gboolean on_draw_light_box(GtkWidget *widget, cairo_t *cr, gpointer data) {
    const char *text = (const char *)data;
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // Définir les dimensions et le style du cadre
    int padding = 10;  // Marge autour du cadre
    int box_width = width - 2 * padding;
    int box_height = height - 2 * padding;

    // Position pour centrer le texte
    int center_x = width / 2;
    int center_y = height / 2;

    // Couleur du cadre lumineux
    double red = 0.2, green = 0.5, blue = 1.0;

    // Effet de lueur extérieure autour du cadre
    for (int i = 0; i < 10; i++) {
        cairo_set_source_rgba(cr, 0.176, 0.18, 0.188, 0.1 * (10 - i));  // Couleur avec transparence décroissante
        cairo_set_line_width(cr, 10 + i * 2);  // Augmente l'épaisseur pour étendre la lueur
        cairo_rectangle(cr, padding, padding, box_width, box_height);
        cairo_stroke(cr);
    }

    // Tracer le cadre principal
    cairo_set_source_rgb(cr, 0.216, 0.22, 0.231);  // Couleur principale
    cairo_set_line_width(cr, 3);  // Épaisseur du cadre
    cairo_rectangle(cr, padding, padding, box_width, box_height);
    cairo_stroke(cr);

    // Texte lumineux au centre du cadre
    cairo_select_font_face(cr, "Courier New", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);

    // Calculer la position du texte pour le centrer
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);
    double text_x = center_x - extents.width / 2;
    double text_y = center_y + extents.height / 2;

    // Ombre pour l'effet de "lueur" autour du texte
    for (int i = 0; i < 5; i++) {
        cairo_set_source_rgba(cr, red, green, blue, 0.2);  // Couleur avec légère transparence
        cairo_move_to(cr, text_x + i - 2, text_y + i - 2);  // Décalage léger pour la lueur
        cairo_show_text(cr, text);
    }

    // Texte principal lumineux
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  // Blanc éclatant pour le texte central
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, text);

    return FALSE;
}

// Fonction pour dessiner la barre de progression circulaire
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;
    double fraction = thread_data->fraction;  // La fraction représente le pourcentage de progression entre 0 et 1
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int radius = MIN(width, height) / 2 -10;  // Rayon du cercle de progression
    int center_x = width / 2;
    int center_y = height / 2;

    // Dessiner le cercle de fond (barre de progression inactive)
    cairo_set_source_rgb(cr, 0.275, 0.29, 0.302); // Couleur de fond gris clair
    cairo_set_line_width(cr, 5);  // Largeur du cercle de fond
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);  // Cercle complet
    cairo_stroke(cr);

      // Créer le dégradé linéaire pour la barre de progression
    cairo_pattern_t *gradient = cairo_pattern_create_linear(center_x - radius, center_y - radius,
                                                           center_x + radius, center_y + radius);
    cairo_pattern_add_color_stop_rgb(gradient, 0.125, 0.961, 0.878, 1.0);  // Début du dégradé (bleu clair)
    cairo_pattern_add_color_stop_rgb(gradient, 1.0, 0.0, 0.3, 0.7);  // Fin du dégradé (bleu foncé)

    // Appliquer le dégradé pour la barre de progression
    cairo_set_line_width(cr, 5);  // Largeur de la barre de progression
    cairo_set_source(cr, gradient);
    cairo_arc(cr, center_x, center_y, radius, -M_PI / 2, -M_PI / 2 + 2 * M_PI * fraction);
    cairo_stroke(cr);

    // Afficher le texte du pourcentage au centre
    cairo_set_source_rgb(cr, 0, 0, 0);  // Couleur du texte (noir)
    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);

    gchar *percentage_text = g_strdup_printf("%.0f%%", fraction * 100);
    cairo_text_extents_t extents;
    cairo_text_extents(cr, percentage_text, &extents);
    cairo_move_to(cr, center_x - extents.width / 2, center_y + extents.height / 2);
    cairo_show_text(cr, percentage_text);
    //g_free(percentage_text);

    return FALSE;
}



// Ajouter GtkDrawingArea à la page 3 pour le graphique
void setup_chart_area(GtkWidget *page) {
    chart_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(chart_area, 700, 500);
    g_signal_connect(G_OBJECT(chart_area), "draw", G_CALLBACK(on_draw_chart), NULL);

    gtk_box_pack_start(GTK_BOX(page), chart_area, TRUE, TRUE, 0);
}

//Chargement du fichier css
static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css", NULL);

    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    g_object_unref(provider);
}


//Fonction pour ajouter des champs pour afficher les messages
static void append_task_message(ThreadData *thread_data, const gchar *message) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(thread_data->message_buffer, &end);
    gtk_text_buffer_insert(thread_data->message_buffer, &end, message, -1);
}


//Fonction pour la mise à jour de la barre de progression
static gboolean update_progress_bar(gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;


   // Vérifier que le widget est valide
    /*if (GTK_WIDGET(thread_data->progress_bar)) {
        // Forcer un redraw pour mettre à jour la barre
        gtk_widget_queue_draw(thread_data->progress_bar);
    } else {
        g_warning("Zone de dessin non valide !");
        return G_SOURCE_REMOVE;
    }*/

    // Vérifier que le widget est valide et visible
    if (GTK_IS_DRAWING_AREA(thread_data->progress_bar)) {
    // Forcer un redraw pour mettre à jour la barre

        gtk_widget_queue_draw(thread_data->progress_bar);
    } else {
        g_warning("Zone de dessin non valide ou invisible !");
        return G_SOURCE_REMOVE;
    }


    if (thread_data->fraction >= 1.0 && !thread_data->is_complete) {
        thread_data->is_complete = TRUE;
        gchar *message = g_strdup_printf("Processus terminé: Liberation de la ressource par le processus: %s\n", thread_data->task_name);
        append_task_message(thread_data, message);
        g_free(message);

        gtk_label_set_text(GTK_LABEL(thread_data->status_label), "Fait");  // Mise à jour de l'état en "Fait"

        used_resources -= thread_data->resource_percentage;  // Libération des ressources

        if(thread_data->use_mutex){
            pthread_mutex_unlock(&resource_mutex);

        }else{
            sem_post(&resource_semaphore);
        }


        return G_SOURCE_REMOVE;
}

    return G_SOURCE_CONTINUE;
}


//Fonction po+ur la simulation de traitement
static void* simulate_thread_processing(void *data) {
    ThreadData *thread_data = (ThreadData *)data;

    if(thread_data->use_mutex){
        pthread_mutex_lock(&resource_mutex);
    }
    // Attente du délai de démarrage si nécessaire
    if (thread_data->start_delay > 0) {
        sleep(thread_data->start_delay);
    }
    gtk_label_set_text(GTK_LABEL(thread_data->status_label), "En cours");

    gchar *message = g_strdup_printf("Processus démarré: Récupération de la ressource par ce processus : %s\n", thread_data->task_name);
    append_task_message(thread_data, message);
    g_free(message);

    for (int i = 0; i <= 100; i++) {
        thread_data->fraction = i / 100.0;
        g_idle_add(update_progress_bar, thread_data);
        usleep(50000*thread_data->memory_time);// Simuler le temps d'attente
    }

    return NULL;
}

// Fonction pour initialiser la zone de dessin et ses signaux
void init_circular_progress_bar(ThreadData *thread_data, GtkWidget *parent_box) {
    // Création et configuration de la zone de dessin pour l'anneau
    thread_data->progress_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(thread_data->progress_bar,100, 100);  // Ajuster la taille de l'anneau

    // Connecte le signal de dessin
    g_signal_connect(G_OBJECT(thread_data->progress_bar), "draw", G_CALLBACK(on_draw), thread_data);

    // Ajoute la zone de dessin dans le conteneur parent
    gtk_box_pack_start(GTK_BOX(parent_box), thread_data->progress_bar, FALSE, FALSE, 0);
    gtk_widget_show(thread_data->progress_bar);
}

// Fonction de démarrage manuel d'une tâche
static void manual_start_task(GtkWidget *button, gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;
    if (!thread_data->is_complete && !thread_data->in_progress &&
        (used_resources + thread_data->resource_percentage) <= max_resources) {
        used_resources += thread_data->resource_percentage;
        thread_data->in_progress = TRUE;

        pthread_t thread;
        if (pthread_create(&thread, NULL, simulate_thread_processing, thread_data) == 0) {
            pthread_detach(thread);
        } else {
            g_warning("Erreur lors de la création du thread pour la tâche : %s", thread_data->task_name);
            used_resources -= thread_data->resource_percentage;
            thread_data->in_progress = FALSE;
        }
    }
}

// Fonction de démarrage automatique basé sur le start_delay
static gboolean auto_start_task(gpointer data) {
    time_t current_time = time(NULL);
    GList *l;

    for (l = task_list; l != NULL; l = l->next) {
        ThreadData *thread_data = (ThreadData *)l->data;
        if (!thread_data->is_complete && !thread_data->in_progress &&
            (used_resources + thread_data->resource_percentage) <= max_resources &&
            (current_time >= thread_data->start_delay)) {
            used_resources += thread_data->resource_percentage;
            thread_data->in_progress = TRUE;


            pthread_t thread;
            if (pthread_create(&thread, NULL, simulate_thread_processing, thread_data) == 0) {
                pthread_detach(thread);
            } else {
                g_warning("Erreur lors de la création du thread pour la tâche : %s", thread_data->task_name);
                used_resources -= thread_data->resource_percentage;
                thread_data->in_progress = FALSE;
            }
        }
    }
    return G_SOURCE_CONTINUE;

}
// Fonction d'initialisation de la barre de progression circulaire
/*static void initialize_progress_bar(ThreadData *thread_data) {
    // Réinitialisation de la fraction de progression à 0
    thread_data->fraction = 0.0;

    // Marquer le processus comme non terminé
    thread_data->is_complete = FALSE;

    // Demander à GTK de redessiner la zone de dessin
    gtk_widget_queue_draw(thread_data->progress_bar);

    // Afficher "0%" au centre de la barre de progression
    gtk_label_set_text(GTK_LABEL(thread_data->status_label), "0%");
}*/

// Fonction d'événement pour dessiner
/*static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;

    // Si le processus est déjà lancé, dessiner la barre de progression
    if (thread_data->fraction > 0) {
        return draw_circular_progress(widget, cr, data);
    }

    // Sinon, dessiner le cercle de base (invisible tant que le chargement n'a pas commencé)
   //return initialize_progress_bar(thread_data);
}*/
GtkWidget *dialog_box(GtkWidget *parent_window, const gchar text){

        GtkWidget *error_text = gtk_message_dialog_new(GTK_BOX(custom_box),GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,text);

        gtk_dialog_run(GTK_DIALOG(error_text));
        gtk_widget_destroy(error_text);

        return error_text;

}

//Fonction pour l'ajout de tâches dans le tableau
ThreadData *add_task(GtkWidget *button, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const gchar *task_name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const gchar *resource_percentage_text = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const gchar *start_delay_text = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const gchar *id_priority_text = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const gchar *memory_time_text;
    double resource_percentage = g_ascii_strtod(resource_percentage_text, NULL);
    int start_delay = g_ascii_strtod(start_delay_text,NULL);
    int id_priority = g_ascii_strtod(id_priority_text,NULL);


    //Ajout des attributs de la nouvelle tâche
    ThreadData *thread_data = g_malloc(sizeof(ThreadData));
    thread_data->task_name = g_strdup(task_name);
    thread_data->resource_percentage = resource_percentage;
    thread_data->start_delay = start_delay;
    thread_data->id_priority = id_priority;
    thread_data->is_complete = FALSE;
    thread_data->in_progress = FALSE;
    thread_data->memory_time =(rand() % 10);
    thread_data->is_scheluded = start_delay >= 0;
    thread_data->use_mutex = FALSE;
    // Crée la zone de dessin pour la barre circulaire
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 50, 50);

    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), thread_data);


    memory_time_text = g_strdup_printf("%d", thread_data->memory_time);
    // Création d'une cellule de nom de tâche avec bordure
    GtkWidget *label1 = gtk_label_new(thread_data->task_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(label1), "grid-cells");

    // Création d'une cellule de statut avec bordure
    thread_data->status_label = gtk_label_new("Pas fait");
    gtk_style_context_add_class(gtk_widget_get_style_context(thread_data->status_label), "grid-cell");

    // Création d'une cellule de pourcentage de ressources avec bordure
    GtkWidget *label3 = gtk_label_new(resource_percentage_text);
    gtk_style_context_add_class(gtk_widget_get_style_context(label3), "grid-cell");

    // Création d'une cellule de temps mémoire avec bordure
    GtkWidget *label4 = gtk_label_new(memory_time_text);
    gtk_style_context_add_class(gtk_widget_get_style_context(label4), "grid-cell");

    GtkWidget *label5 = gtk_label_new(id_priority_text);
    gtk_style_context_add_class(gtk_widget_get_style_context(label5), "grid-cell");

    if(g_strcmp0(task_name,"") == 0 /*|| resource_percentage_text == NULL || id_priority_text == NULL || start_delay_text == NULL || *task_name == "\0" || *resource_percentage_text == "\0" || *id_priority_text == "\0" || *start_delay_text == "\0"*/){
 // Afficher un message d'erreur si l'entrée est vide
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(button)),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Erreur : l'entrée est vide !");
        gtk_widget_set_name(dialog,"box-diag");
        gtk_window_set_modal(GTK_WINDOW(dialog), FALSE); // Rendre le dialogue non-bloquant
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_widget_show(dialog);

    }else{
    // Ajout des widgets dans la grille
    gtk_grid_attach(GTK_GRID(grid), label1, 0, current_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), thread_data->status_label, 1, current_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label3, 2, current_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label4, 3, current_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label5, 4, current_row, 1, 1);
    }

    //thread_data->progress_bar = gtk_progress_bar_new();
    //gtk_widget_set_name(thread_data->progress_bar,"progress-bar");

   GtkWidget *parent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

   init_circular_progress_bar(thread_data,parent_box);

    //gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(thread_data->progress_bar), TRUE);

    //initialize_progress_bar(thread_data);

    GtkWidget *task_label = gtk_label_new(thread_data->task_name);
    //gtk_grid_attach(GTK_GRID(progress_grid), task_label, 0, current_progress_row, 1, 1);
    //gtk_grid_attach(GTK_GRID(progress_grid), drawing_area, 0, current_progress_row + 1, 1, 1);

    gtk_grid_attach(GTK_GRID(progress_grid), parent_box, 0, current_progress_row , 1, 1);


    if(thread_data->start_delay>15){
        thread_data->launch_button = gtk_button_new_with_label("Lancer");
        gtk_widget_set_name(thread_data->launch_button, "button-3D");
        g_signal_connect(thread_data->launch_button, "clicked" , G_CALLBACK(manual_start_task), thread_data);
        gtk_grid_attach(GTK_GRID(grid), thread_data->launch_button, 5, current_row, 1, 1);
    }

    g_timeout_add(thread_data->start_delay, (GSourceFunc)auto_start_task, thread_data);


    thread_data->message_view = gtk_text_view_new();
    gtk_widget_set_name(thread_data->message_view,"table-view");
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(thread_data->message_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(thread_data->message_view), FALSE);
    thread_data->message_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(thread_data->message_view));
    gtk_widget_set_size_request(thread_data->message_view, 250, 50);

    gtk_grid_attach(GTK_GRID(progress_grid), thread_data->message_view, 1, current_progress_row , 1, 1);

    //Partir à la ligne suivante
    current_row++;
    current_progress_row += 3;

    gtk_widget_show_all(grid);
    gtk_widget_show_all(progress_grid);

    //Effacer le contenu après ajout des tâches
    gtk_entry_set_text(GTK_ENTRY(entries[0]), "");
    gtk_entry_set_text(GTK_ENTRY(entries[1]), "");
    gtk_entry_set_text(GTK_ENTRY(entries[2]), "");
    gtk_entry_set_text(GTK_ENTRY(entries[3]), "");

    //Ajout des élements à la liste
    task_list = g_list_append(task_list, thread_data);
    return thread_data;
}




void using_mutex(gpointer data){
    ThreadData *thread_data = (ThreadData *)data;

    thread_data->use_mutex = TRUE;

}

void no_using_mutex(gpointer data){
    ThreadData *thread_data = (ThreadData*)data;

    thread_data->use_mutex = FALSE;

}

// Fonction d'animation
gboolean animate_widget(gpointer data) {
    AnimationData *anim_data = (AnimationData *)data;

    // Mise à jour de la position et de l'opacité
    gtk_fixed_move(GTK_FIXED(gtk_widget_get_parent(anim_data->widget)), anim_data->widget, 50, anim_data->y_start);
    gtk_widget_set_opacity(anim_data->widget, anim_data->opacity);

    // Mise à jour pour l'étape suivante
    anim_data->y_start -= anim_data->step;
    anim_data->opacity += 0.05;

    // Fin de l'animation
    if (anim_data->y_start <= anim_data->y_end || anim_data->opacity >= 1.0) {
        gtk_widget_set_opacity(anim_data->widget, 1.0); // Assurer pleine opacité
        return FALSE; // Arrêter l'animation
    }

    return TRUE; // Continuer l'animation
}

//Fonction pour basculer la visibilité e la boite
void toggle_box_visibility_false(GtkWidget *widget, gpointer data){
    GtkWidget *box = GTK_WIDGET(data);
    gtk_widget_hide(box);

}
void toggle_box_visibility_true(GtkWidget *widget, gpointer data){
    GtkWidget *box = GTK_WIDGET(data);

    /*AnimationData *anim_data = g_new(AnimationData, 1);
    anim_data->widget = box;
    anim_data->y_start = 300; // Position de départ en y (en dehors de la vue)
    anim_data->y_end = 150;   // Position finale
    anim_data->step = 5;      // Nombre de pixels par étape
    anim_data->opacity = 0.0; // Opacité de départ*/

    gtk_widget_show(box);

}

//Fonction poir creer une boite

GtkWidget *create_custom_box(){
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_widget_set_name(box ,"box");

    return box;

}




int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    initialize_start_time();

    //Initialisation du semaphore et du mutex
    pthread_mutex_init(&resource_mutex,NULL);
    sem_init(&resource_semaphore, 0, 2);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de Tâches");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 700, 700);


    GtkWidget *overlay = gtk_overlay_new();
    GtkWidget *overlay2 = gtk_overlay_new();
    GtkWidget *notebook = gtk_notebook_new();


    GtkWidget *page2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(page2,"notebook-page-2");
     GtkWidget *page3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(page3,"notebook-page-3");
     GtkWidget *page4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(page4,"notebook-page-2");

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *scrolled_window2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);



    grid = gtk_grid_new();
    grid2 = gtk_grid_new();

    // Enlever l'espacement pour que les cellules se touchent
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 0);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 0);

    // Enlever l'espacement pour que les cellules se touchent
    gtk_grid_set_column_homogeneous(GTK_GRID(grid2), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid2), 0);
    gtk_grid_set_column_spacing(GTK_GRID(grid2), 0);

    GtkWidget *title_page2 = gtk_label_new("Tableaux des processus");
    gtk_widget_set_name(title_page2,"label-title");
    gtk_box_pack_start(GTK_BOX(page2),title_page2, FALSE, FALSE, 0);


    GtkWidget *title_name = gtk_label_new("Nom du processus");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_name), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid), title_name, 0, 0, 1, 1);

    GtkWidget *title_status_for_process = gtk_label_new("État");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_status_for_process), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid), title_status_for_process, 1, 0, 1, 1);

    GtkWidget *title_memory = gtk_label_new("Mémoire(%)");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_memory), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid), title_memory, 2, 0, 1, 1);

    GtkWidget *title_time = gtk_label_new("Temps Mémoire");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_time), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid), title_time, 3, 0, 1, 1);

    GtkWidget *title_priority = gtk_label_new("Priorité de la tâche(1, 2 ou 3)");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_priority), "grid-cells");
    gtk_grid_attach(GTK_GRID(grid), title_priority, 4, 0, 1, 1);

    GtkWidget *title_page3 = gtk_label_new("Tableaux des processus");
    gtk_widget_set_name(title_page3,"label-title");
    gtk_box_pack_start(GTK_BOX(page4),title_page3, FALSE, FALSE, 0);



     // Création des boutons radio
    GtkWidget *radio_sem = gtk_radio_button_new_with_label(NULL, "Semaphore");
    g_signal_connect(radio_sem, "toggled", G_CALLBACK(no_using_mutex), NULL);


    GtkWidget *radio_mutex = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_sem), "Mutex");
    g_signal_connect(radio_mutex, "toggled", G_CALLBACK(using_mutex), NULL);

       // Création des boutons radio
    GtkWidget *radio_sem2 = gtk_radio_button_new_with_label(NULL, "Semaphore");
    g_signal_connect(radio_sem2, "toggled", G_CALLBACK(no_using_mutex), NULL);


    GtkWidget *radio_mutex2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_sem), "Mutex");
    g_signal_connect(radio_mutex2, "toggled", G_CALLBACK(using_mutex), NULL);

    custom_box = create_custom_box();
    gtk_widget_set_name(custom_box,"box");
    GtkWidget *custom_box2 = create_custom_box();
    gtk_widget_set_name(custom_box2,"box");

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    GtkWidget *hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);

    GtkWidget *label = gtk_label_new("Ajout de tâches");
    gtk_widget_set_name(label, "label-title");
    GtkWidget *label2 = gtk_label_new("Ajout de tâches");
    gtk_widget_set_name(label2, "label-title");

    gtk_box_pack_start(GTK_BOX(hbox2),label, FALSE, FALSE,0);
    gtk_box_pack_start(GTK_BOX(custom_box2),label2, FALSE, FALSE,0);

    GtkWidget *entry2 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry2), "Nom du processus");
    gtk_widget_set_name(entry2,"entry");
    gtk_box_pack_start(GTK_BOX(hbox2), entry2, FALSE, FALSE, 0);

    GtkWidget *entry3 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry3), "Espace mémoire de 0.0 - 100.0");
    gtk_widget_set_name(entry3,"entry");
    gtk_box_pack_start(GTK_BOX(hbox2), entry3, FALSE, FALSE, 0);

    GtkWidget *entry4 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry4), "Durée avant lancement");
    gtk_widget_set_name(entry4,"entry");
    gtk_box_pack_start(GTK_BOX(hbox2), entry4, FALSE, FALSE, 0);

    GtkWidget *entry5= gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry5), "Priorité de la tâche");
    gtk_widget_set_name(entry5,"entry");
    gtk_box_pack_start(GTK_BOX(hbox2), entry5, FALSE, FALSE, 0);


    GtkWidget *entry6 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry6), "Nom de la tâche");
    gtk_widget_set_name(entry6,"entry");
    gtk_box_pack_start(GTK_BOX(custom_box2), entry6, FALSE, FALSE, 0);

    GtkWidget *entry7 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry7), "Durée avant lancement");
    gtk_widget_set_name(entry7,"entry");
    gtk_box_pack_start(GTK_BOX(custom_box2), entry7, FALSE, FALSE, 0);



    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 250, 200);  // Taille du cadre

    // Connecter le signal "draw" pour dessiner le cadre lumineux
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_light_box), "Gestion des tâches");



    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE,0);

      // Ajouter la zone de dessin dans une box ou autre conteneur
    gtk_box_pack_end(GTK_BOX(hbox), drawing_area, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_box),hbox, FALSE, FALSE,0);


    GtkWidget *entries1[4] = {entry2, entry3, entry4, entry5};
    GtkWidget *entries2[2] = {entry6, entry7};
    GtkWidget *label6 = gtk_label_new("Choix de l'outil de synchronisation");
    gtk_widget_set_name(label6, "label-synchro");
    gtk_box_pack_start(GTK_BOX(custom_box), label6, FALSE, FALSE,0);


    GtkWidget *label7 = gtk_label_new("Choix de l'outil de synchronisation");
    gtk_widget_set_name(label7, "label-synchro");
    gtk_box_pack_start(GTK_BOX(custom_box2), label7, FALSE, FALSE,0);

    gtk_box_pack_start(GTK_BOX(hbox3), radio_sem, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox3), radio_mutex, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox4), radio_sem2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox4), radio_mutex2, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(custom_box), hbox3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_box2), hbox4, FALSE, FALSE, 0);


    //Bouton d'Ajout des tâches
    GtkWidget *add_button = gtk_button_new_with_label("Ajouter un processus");
    gtk_widget_set_name(add_button,"button-3D");
    g_signal_connect(add_button, "clicked", G_CALLBACK(add_task), entries1);

     //Bouton d'Ajout des tâches
    GtkWidget *add_button2 = gtk_button_new_with_label("Ajouter une tâche");
    gtk_widget_set_name(add_button2,"button-3D");
    g_signal_connect(add_button2, "clicked", G_CALLBACK(add_tache), entries2);

     //Bouton d'Ajout des tâches
    GtkWidget *launch_tache = gtk_button_new_with_label("Lancer une tâche");
    gtk_widget_set_name(launch_tache,"button-3D");
    g_signal_connect(launch_tache, "clicked", G_CALLBACK(auto_start_tache), NULL);

    gtk_container_add(GTK_CONTAINER(scrolled_window2), grid2);
    gtk_box_pack_start(GTK_BOX(page4), scrolled_window2, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);
    gtk_box_pack_start(GTK_BOX(page2), scrolled_window, TRUE, TRUE, 0);

    GtkWidget *toggle_button = gtk_button_new_with_label("+");
    gtk_widget_set_name( toggle_button, "button-3D");
    g_signal_connect(toggle_button, "clicked", G_CALLBACK(toggle_box_visibility_true),custom_box);

    GtkWidget *toggle_button2 = gtk_button_new_with_label("x");
    gtk_widget_set_name( toggle_button2, "button-3D");
    g_signal_connect(toggle_button2, "clicked", G_CALLBACK(toggle_box_visibility_false),custom_box);

    GtkWidget *toggle_button3 = gtk_button_new_with_label("+");
    gtk_widget_set_name( toggle_button3, "button-3D");
    g_signal_connect(toggle_button3, "clicked", G_CALLBACK(toggle_box_visibility_true),custom_box2);

    GtkWidget *toggle_button4 = gtk_button_new_with_label("x");
    gtk_widget_set_name( toggle_button4, "button-3D");
    g_signal_connect(toggle_button4, "clicked", G_CALLBACK(toggle_box_visibility_false),custom_box2);


    gtk_box_pack_end(GTK_BOX(custom_box), toggle_button2, FALSE, FALSE,0);
    gtk_box_pack_end(GTK_BOX(custom_box2), toggle_button4, FALSE, FALSE,0);
    gtk_box_pack_end(GTK_BOX(custom_box), add_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(custom_box2), add_button2, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page2), toggle_button, FALSE, FALSE,0);
    gtk_box_pack_end(GTK_BOX(page4), toggle_button3, FALSE, FALSE,0);
    gtk_box_pack_end(GTK_BOX(page4), launch_tache, FALSE, FALSE,0);



    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), page2);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), custom_box);

     gtk_overlay_add_overlay(GTK_OVERLAY(overlay2), page4);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay2), custom_box2);



    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), overlay, gtk_label_new("Tableau des tâches"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), overlay2, gtk_label_new("Systme des tâches"));

     // Ajout de la page de progression avec la barre de défilement
    GtkWidget *scrolled_progress_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_progress_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_progress_window, 580, 450);


    progress_grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(progress_grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(progress_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(progress_grid), 10);
    gtk_container_add(GTK_CONTAINER(scrolled_progress_window), progress_grid);


    // Ajoutez un conteneur vertical sur la page "Suivie des tâches"
    GtkWidget *progress_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(progress_box,"notebook-page-3");
    gtk_box_pack_start(GTK_BOX(progress_box), scrolled_progress_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(progress_box), progress_grid, TRUE, TRUE, 0);


    // Attachez ce conteneur à la page "Suivie des tâches"
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), progress_box, gtk_label_new("Suivi des tâches"));

    setup_chart_area(page3);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page3, gtk_label_new("Diagramme"));




    gtk_container_add(GTK_CONTAINER(window), notebook);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    //Chargement du style de l'interface
    load_css();

    //Chargement des widgets
    gtk_widget_show_all(window);


    gtk_widget_hide(custom_box);
    gtk_widget_hide(custom_box2);

    gtk_main();

    pthread_mutex_destroy(&resource_mutex);
    sem_destroy(&resource_semaphore);

    return 0;


}
/*static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;
    double fraction = thread_data->fraction;
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int radius = MIN(width, height) / 2 -3;  // Ajustement du rayon
    int center_x = width / 2;
    int center_y = height / 2;

    // Dessiner le cercle de fond (barre de progression inactive)
    cairo_set_source_rgb(cr, 0.275, 0.29, 0.302); // Gris clair
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_fill(cr);

    // Dessiner la barre de progression
    cairo_set_source_rgb(cr, 0.0, 0.5, 1.0);  // Couleur bleue pour la progression
    cairo_arc(cr, center_x, center_y, radius, -M_PI / 2, -M_PI / 2 + 2 * M_PI * fraction);
    cairo_line_to(cr, center_x, center_y);  // Fermer le chemin vers le centre
    cairo_fill(cr);

    // Bordure externe pour plus de visibilité
    /*cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);  // Couleur gris clair pour la bordure
    cairo_arc(cr, center_x, center_y, radius + 2, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Afficher le texte du pourcentage au centre
    cairo_set_source_rgb(cr, 0, 0, 0);  // Couleur du texte (noir)
    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);

    gchar *percentage_text = g_strdup_printf("%.0f%%", fraction * 100);
    cairo_text_extents_t extents;
    cairo_text_extents(cr, percentage_text, &extents);
    cairo_move_to(cr, center_x - extents.width / 2, center_y + extents.height / 2);
    cairo_show_text(cr, percentage_text);
    g_free(percentage_text);

    return FALSE;
}*/



// Fonction de dessin pour la barre de chargement circulaire
/*gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer data) {
    CircularProgressData *progress_data = (CircularProgressData *) data;

    // Obtenez les dimensions du widget
    int width, height;
    gtk_widget_get_size_request(widget, &width, &height);

    // Trouver le centre et le rayon du cercle
    double center_x = width / 2.0;
    double center_y = height / 2.0;
    double radius = MIN(width, height) / 3.0;

    // Couleur de fond
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9); // gris clair
    cairo_paint(cr);

    // Dessiner le cercle de fond
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * G_PI);
    cairo_set_line_width(cr, 10);
    cairo_stroke(cr);

    // Dessiner la progression circulaire
    cairo_set_source_rgb(cr, 0.2, 0.6, 1.0); // bleu
    cairo_arc(cr, center_x, center_y, radius, -G_PI / 2, -G_PI / 2 + 2 * G_PI * progress_data->progress);
    cairo_stroke(cr);

    // Afficher le pourcentage au centre
    cairo_set_source_rgb(cr, 0, 0, 0); // noir
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, radius / 2);

    char text[10];
    snprintf(text, sizeof(text), "%.0f%%", progress_data->progress * 100);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);
    cairo_move_to(cr, center_x - extents.width / 2, center_y + extents.height / 2);
    cairo_show_text(cr, text);

    return FALSE;
}*/
// Fonction de dessin de la barre circulaire
/*static gboolean draw_circular_progress(GtkWidget *widget, cairo_t *cr, gpointer data) {
    ThreadData *thread_data = (ThreadData *)data;

    int width, height;
    gtk_widget_get_size_request(widget, &width, &height);

    double center_x = width / 2.0;
    double center_y = height / 2.0;
    double radius = MIN(width, height) / 3.0;

    // Fond de la barre
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_paint(cr);

    // Cercle de fond
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * G_PI);
    cairo_set_line_width(cr, 10);
    cairo_stroke(cr);

    // Progression circulaire
    cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);
    cairo_arc(cr, center_x, center_y, radius, -G_PI / 2, -G_PI / 2 + 2 * G_PI * thread_data->fraction);
    cairo_stroke(cr);

    // Afficher le pourcentage au centre
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, radius / 2);

    char text[10];
    snprintf(text, sizeof(text), "%.0f%%", thread_data->fraction * 100);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);
    cairo_move_to(cr, center_x - extents.width / 2, center_y + extents.height / 2);
    cairo_show_text(cr, text);

    return FALSE;
}*/




