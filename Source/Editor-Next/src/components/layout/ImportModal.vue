<template>
  <n-modal :show="assetStore.isImporting" :mask-closable="false" transform-origin="center">
    <n-card
      style="width: 400px"
      title="Processing Assets"
      :bordered="false"
      size="huge"
      role="dialog"
      aria-modal="true"
    >
      <n-space vertical>
        <n-text depth="3">Converting: {{ assetStore.currentImportName }}</n-text>
        <n-progress
          type="line"
          :percentage="assetStore.importProgress"
          :indicator-placement="'inside'"
          processing
        />
      </n-space>
    </n-card>
  </n-modal>
</template>

<script setup>
import { NModal, NCard, NSpace, NText, NProgress } from 'naive-ui'
import { assetStore } from '../../store/assetStore'
</script>

<style scoped>
/* Scoped styles for the import modal if needed */
</style>
