// =============================================================
// Projeto: Monitorização de Filtro de Água com IoT
// Disciplina: Internet of Things (IoT) — MEEC / IPB
// Bucket: IoT_LAB
// Measurement: filtro_agua
// Descrição: Query principal do dashboard Grafana.
//            Recupera os dados de pressão diferencial, caudal,
//            pressões absolutas e estado de saturação do filtro,
//            agregados pela janela temporal seleccionada.
// =============================================================

from(bucket: "IoT_LAB")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)

  // Selecciona apenas os dados do sensor de filtro de água
  |> filter(fn: (r) => r["_measurement"] == "filtro_agua")

  // Campos monitorizados:
  //   dp_kpa    — pressão diferencial entre P1 e P2 (kPa)
  //   flow_pct  — caudal estimado (% da escala)
  //   p1_kpa    — pressão na entrada do filtro (kPa)
  //   p2_kpa    — pressão na saída do filtro (kPa)
  //   saturado  — flag de saturação (0 = OK, 1 = SATURADO)
  |> filter(fn: (r) =>
      r["_field"] == "dp_kpa"   or
      r["_field"] == "flow_pct" or
      r["_field"] == "p1_kpa"   or
      r["_field"] == "p2_kpa"   or
      r["_field"] == "saturado"
  )

  // Filtra por estado operacional reportado pelo ESP32
  |> filter(fn: (r) => r["estado"] == "OK" or r["estado"] == "SATURADO")

  // Agrega pela janela temporal do dashboard (média)
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)

  |> yield(name: "mean")
